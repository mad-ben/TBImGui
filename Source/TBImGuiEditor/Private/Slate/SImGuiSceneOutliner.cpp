// Copyright Lewis Scrivens. All Rights Reserved.
#include "Slate/SImGuiSceneOutliner.h"

#include "ActorTreeItem.h"
#include "Editor.h"
#include "FolderTreeItem.h"
#include "WorldTreeItem.h"
#include "Framework/Application/SlateApplication.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "ISceneOutlinerColumn.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ISceneOutlinerMode.h"
#include "ISceneOutlinerTreeItem.h"
#include "Layout/WidgetPath.h"
#include "ScopedTransaction.h"
#include "SSceneOutliner.h"
#include "SceneOutlinerDragDrop.h"
#include "Styling/AppStyle.h"
#include "TBImGuiEditor.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

namespace TBIMGUI_SCENE_OUTLINER
{
static FString GetColumnLabel(const SHeaderRow::FColumn& Column)
{
	if (Column.ColumnId == FSceneOutlinerBuiltInColumnTypes::Gutter())
	{
		return TEXT("V");
	}
	if (Column.ColumnId == FSceneOutlinerBuiltInColumnTypes::Pinned())
	{
		return TEXT("P");
	}
	const FString Label = Column.DefaultText.Get(FText::GetEmpty()).ToString();
	return Label.IsEmpty() || Label.EndsWith(TEXT("[LabelMissing]")) ? Column.ColumnId.ToString() : Label;
}

static TOptional<bool> GetItemVisibility(const ISceneOutlinerTreeItem& Item)
{
	if (Item.HasVisibilityInfo())
	{
		return Item.GetVisibility();
	}
	TOptional<bool> Result;
	for (const auto& WeakChild : Item.GetChildren())
	{
		if (const auto Child = WeakChild.Pin())
		{
			if (const TOptional<bool> ChildVisible = GetItemVisibility(*Child); ChildVisible.IsSet())
			{
				Result = Result.Get(false) || ChildVisible.GetValue();
			}
		}
	}
	return Result;
}

static void SetItemVisibility(ISceneOutlinerTreeItem& Item, bool bVisible)
{
	Item.OnVisibilityChanged(bVisible);
	for (const auto& WeakChild : Item.GetChildren())
	{
		if (const auto Child = WeakChild.Pin())
		{
			SetItemVisibility(*Child, bVisible);
		}
	}
}

static bool GetItemPinned(const ISceneOutlinerTreeItem& Item)
{
	if (Item.HasPinnedStateInfo())
	{
		return Item.GetPinnedState();
	}
	for (const auto& WeakChild : Item.GetChildren())
	{
		if (const auto Child = WeakChild.Pin(); Child && GetItemPinned(*Child))
		{
			return true;
		}
	}
	return false;
}
} // namespace TBIMGUI_SCENE_OUTLINER

void SImGuiSceneOutliner::Construct(const FArguments& InArgs)
{
	check(InArgs._Outliner.IsValid());
	WeakOutliner = InArgs._Outliner;
	FTBImGuiContextParams Params(FString::Printf(TEXT("Outlnr%08x"), GetTypeHash(InArgs._Outliner->GetOutlinerIdentifier())));
	Params.ViewportMode = ETBImGuiViewportMode::Single;
	Params.bDockingSupport = false;
	SImGuiContextWidget::Construct(SImGuiContextWidget::FArguments().ContextParams(Params));
}

void SImGuiSceneOutliner::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
	if (const auto Item = ActiveRename.Pin(); Item && (!Item->IsValid() || !Rename.IsActive()))
	{
		Rename.Cancel();
		ActiveRename.Reset();
	}
	if (const auto Item = DragHoverItem.Pin(); Item && FSlateApplication::Get().IsDragDropping() && CurrentTime - DragHoverStart > 0.75)
	{
		if (const auto Outliner = WeakOutliner.Pin())
		{
			Outliner->SetItemExpansion(Item, true);
		}
	}
	SetContentSize(Geometry.GetLocalSize());
	bRenameDrawn = false;
	SImGuiContextWidget::Tick(Geometry, CurrentTime, DeltaTime);
	if (Rename.IsActive() && !bRenameDrawn)
	{
		Rename.Cancel();
	}
	Rename.Flush();
	// Selection, menus, and hierarchy edits may mutate the model. Finish drawing first.
	TArray<TFunction<void()>> Actions = MoveTemp(PendingActions);
	PendingActions.Reset();
	for (auto& Action : Actions)
	{
		Action();
	}
	FTBImGuiContextScope Scope(OwningContext);
	if (bVisibilityPainting && (bEndVisibilityPainting || !Scope.IsValid() || !ImGui::IsMouseDown(ImGuiMouseButton_Left)))
	{
		VisibilityTransaction.Reset();
		VisibilityPaintedItems.Reset();
		bVisibilityPainting = false;
	}
	bEndVisibilityPainting = false;
}

FReply SImGuiSceneOutliner::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	FTBImGuiContextScope Scope(OwningContext);
	FReply ImGuiReply = SImGuiContextWidget::OnKeyDown(Geometry, Event);
	if (Scope.IsValid() && !ImGui::GetIO().WantTextInput)
	{
		if (const auto Outliner = WeakOutliner.Pin())
		{
			// Keep Delete, F2, clipboard, duplicate, undo, and mode shortcuts on Unreal's command path.
			FReply Reply = Outliner->OnKeyDown(Geometry, Event);
			if (Reply.IsEventHandled())
			{
				return Reply;
			}
		}
		const FKey Key = Event.GetKey();
		if (Key == EKeys::F10 && Event.IsShiftDown())
		{
			ShowContextMenu();
			return FReply::Handled();
		}
		if (Key == EKeys::SpaceBar && (Event.IsControlDown() || Event.IsCommandDown()))
		{
			const int32 Index = Rows.IndexOfByPredicate([this](const FRow& Row)
			                                            { return Row.Item == NavigationItem.Pin(); });
			SelectRow(Index, true, false, ESelectInfo::OnKeyPress);
			return FReply::Handled();
		}
		if (Key == EKeys::Enter)
		{
			if (const auto Item = NavigationItem.Pin())
			{
				PendingActions.Add([Weak = WeakOutliner, Item]()
				                   {
					if (const auto Backend = Weak.Pin(); Backend && Item->IsValid())
					{
						Backend->ActivateCustomTreeItem(Item);
					} });
			}
			return FReply::Handled();
		}
		const bool bTreeNavigation = Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Home || Key == EKeys::End || Key == EKeys::PageUp || Key == EKeys::PageDown || ((Event.IsControlDown() || Event.IsCommandDown()) && Key == EKeys::A);
		// Feed ImGui's modifier state, but let unbound editor shortcuts bubble to the editor.
		return bTreeNavigation ? FReply::Handled() : FReply::Unhandled();
	}
	return ImGuiReply;
}

FReply SImGuiSceneOutliner::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	FReply Reply = SImGuiContextWidget::OnMouseButtonDown(Geometry, Event);
	if (Reply.IsEventHandled())
	{
		Scroll.MouseDown(Event);
		Reply.SetUserFocus(AsShared(), EFocusCause::Mouse);
		DragSourceItem.Reset();
		for (const FHitItem& Hit : HitItems)
		{
			if (Hit.NameRect.ContainsPoint(Event.GetScreenSpacePosition()))
			{
				DragSourceItem = Hit.Item;
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton && !Rename.IsActive())
				{
					Reply.DetectDrag(AsShared(), EKeys::LeftMouseButton);
				}
				break;
			}
		}
	}
	return Reply;
}

void SImGuiSceneOutliner::AppendRows(const TArrayView<const FSceneOutlinerTreeItemPtr>& Items, int32 Depth)
{
	const auto Outliner = WeakOutliner.Pin();
	for (const auto& Item : Items)
	{
		if (!Item.IsValid() || !Item->IsValid())
		{
			continue;
		}
		TArray<FSceneOutlinerTreeItemPtr> Children;
		Outliner->GetChildrenForCustomTree(Item, Children);
		Rows.Add({Item, Depth, !Children.IsEmpty()});
		// Unreal's rename command and new-folder action both use this event.
		Item->RenameRequestEvent.BindSP(this, &SImGuiSceneOutliner::ShowRenameEditor, TWeakPtr<ISceneOutlinerTreeItem>(Item));
		if (!Children.IsEmpty() && Outliner->IsItemExpanded(Item))
		{
			AppendRows(Children, Depth + 1);
		}
	}
}

void SImGuiSceneOutliner::SelectRow(int32 Index, bool bToggle, bool bRange, ESelectInfo::Type SelectInfo)
{
	const auto Outliner = WeakOutliner.Pin();
	if (!Outliner || !Rows.IsValidIndex(Index))
	{
		return;
	}
	const FSceneOutlinerTreeItemPtr Item = Rows[Index].Item;
	if (!Item->CanInteract() || !Item->Flags.bInteractive || Outliner->GetMode()->GetSelectionMode() == ESelectionMode::None)
	{
		return;
	}
	bToggle &= Outliner->GetMode()->GetSelectionMode() == ESelectionMode::Multi;
	bRange &= Outliner->GetMode()->GetSelectionMode() == ESelectionMode::Multi;
	if (SelectInfo == ESelectInfo::OnMouseClick && !bToggle && !bRange && SelectedItems.Num() > 1 && SelectedItems.Contains(Item.Get()) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		NavigationItem = Item;
		SelectionAnchor = Item;
		return;
	}
	TArray<FSceneOutlinerTreeItemPtr> Items;
	if (bRange)
	{
		const auto Anchor = SelectionAnchor.Pin();
		const int32 AnchorIndex = Rows.IndexOfByPredicate([&](const FRow& Row)
		                                                  { return Row.Item == Anchor; });
		if (AnchorIndex != INDEX_NONE)
		{
			for (int32 Row = FMath::Min(Index, AnchorIndex); Row <= FMath::Max(Index, AnchorIndex); ++Row)
			{
				if (Rows[Row].Item->CanInteract() && Rows[Row].Item->Flags.bInteractive)
				{
					Items.Add(Rows[Row].Item);
				}
			}
		}
	}
	if (Items.IsEmpty())
	{
		Items.Add(Item);
	}
	const bool bDeselect = (bToggle || Outliner->GetMode()->GetSelectionMode() == ESelectionMode::SingleToggle) && !bRange && SelectedItems.Contains(Item.Get());
	NavigationItem = Item;
	PendingActions.Add([Weak = WeakOutliner, Items = MoveTemp(Items), Item, bToggle, bDeselect, SelectInfo]()
	                   {
		if (const auto Backend = Weak.Pin())
		{
			Backend->SetCustomTreeSelection(Items, !bDeselect, !bToggle, SelectInfo);
			if (SelectInfo == ESelectInfo::OnMouseClick)
			{
				Backend->ClickCustomTreeItem(Item);
			}
		} });
	if (!bRange)
	{
		SelectionAnchor = Item;
	}
}

void SImGuiSceneOutliner::ShowContextMenu()
{
	const TSharedPtr<SSceneOutliner> Outliner = WeakOutliner.Pin();
	if (!Outliner || !Outliner->GetTreeView() || !FSlateApplication::IsInitialized())
	{
		return;
	}
	FWidgetPath OwnerPath;
	if (!FSlateApplication::Get().GeneratePathToWidgetUnchecked(AsShared(), OwnerPath) || !OwnerPath.IsValid())
	{
		return;
	}
	const TSharedPtr<SWidget> Menu = Outliner->CreateCustomTreeContextMenu();
	if (Menu && FSlateApplication::Get().GeneratePathToWidgetUnchecked(AsShared(), OwnerPath) && OwnerPath.IsValid())
	{
		FSlateApplication::Get().PushMenu(AsShared(), OwnerPath, Menu.ToSharedRef(),
		                                  FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
	}
}

void SImGuiSceneOutliner::ShowRenameEditor(TWeakPtr<ISceneOutlinerTreeItem> WeakItem)
{
	Rename.Cancel();
	const auto Item = WeakItem.Pin();
	const auto Outliner = WeakOutliner.Pin();
	if (!Outliner || !Item || !Item->IsValid() || !Outliner->CanExecuteRenameRequest(*Item))
	{
		return;
	}
	const auto Row = SNew(STableRow<FSceneOutlinerTreeItemPtr>, Outliner->GetTreeView().ToSharedRef());
	Row->SetContent(Item->GenerateLabelWidget(*Outliner, *Row));
	if (Rename.Begin(Row))
	{
		ActiveRename = Item;
		FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::SetDirectly);
	}
	Item->RenameRequestEvent.BindSP(this, &SImGuiSceneOutliner::ShowRenameEditor, WeakItem);
}

void SImGuiSceneOutliner::ShowColumnEditor(FName ColumnId, FSceneOutlinerTreeItemPtr Item)
{
	const auto Outliner = WeakOutliner.Pin();
	if (!Outliner)
	{
		return;
	}
	TSharedPtr<SWidget> Content;
	if (Item)
	{
		if (const auto Column = Outliner->GetColumns().FindRef(ColumnId))
		{
			const auto Row = SNew(STableRow<FSceneOutlinerTreeItemPtr>, Outliner->GetTreeView().ToSharedRef());
			Row->SetContent(Column->ConstructRowWidget(Item.ToSharedRef(), *Row));
			Content = Row;
		}
	}
	else
	{
		for (const auto& Column : Outliner->GetTreeView()->GetHeaderRow()->GetColumns())
		{
			if (Column.ColumnId == ColumnId)
			{
				if (Column.OnGetMenuContent.IsBound())
				{
					Content = Column.OnGetMenuContent.Execute();
				}
				else if (const auto NativeColumn = Outliner->GetColumns().FindRef(ColumnId))
				{
					Content = SNew(SHeaderRow) + NativeColumn->ConstructHeaderRowColumn();
				}
				break;
			}
		}
	}
	if (Content)
	{
		const auto Popup = SNew(SBox).MinDesiredWidth(180.0f).Padding(6.0f)[Content.ToSharedRef()];
		FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Popup,
		                                  FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
	}
}

FSceneOutlinerTreeItemPtr SImGuiSceneOutliner::ItemAt(FVector2D Position) const
{
	const TSharedPtr<SSceneOutliner> Outliner = WeakOutliner.Pin();
	if (!Outliner)
	{
		return nullptr;
	}
	for (const FHitItem& Hit : HitItems)
	{
		if (Hit.Rect.ContainsPoint(Position) && Hit.Item && Hit.Item->IsValid() && Outliner->GetTreeItem(Hit.Item->GetID()) == Hit.Item)
		{
			return Hit.Item;
		}
	}
	return nullptr;
}

TSharedPtr<IToolTip> SImGuiSceneOutliner::GetToolTip()
{
	const auto Item = ItemAt(FSlateApplication::Get().GetCursorPos());
	const auto Outliner = WeakOutliner.Pin();
	if (!Item || !Outliner || Rename.IsActive() || Scroll.IsPanning())
	{
		return nullptr;
	}
	if (ToolTipItem.Pin() != Item || !ToolTipSource)
	{
		const auto Row = SNew(STableRow<FSceneOutlinerTreeItemPtr>, Outliner->GetTreeView().ToSharedRef());
		Row->SetContent(Item->GenerateLabelWidget(*Outliner, *Row));
		ToolTipSource = Row;
		ToolTipItem = Item;
		Item->RenameRequestEvent.BindSP(this, &SImGuiSceneOutliner::ShowRenameEditor, TWeakPtr<ISceneOutlinerTreeItem>(Item));
	}
	const auto Editor = FTBImGuiInlineRename::FindEditor(ToolTipSource.ToSharedRef());
	return Editor ? Editor->GetToolTip() : ToolTipSource->GetToolTip();
}
FReply SImGuiSceneOutliner::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	FReply Reply = SImGuiContextWidget::OnMouseMove(Geometry, Event);
	return Scroll.MouseMove(Geometry, Event) ? FReply::Handled() : Reply;
}

void SImGuiSceneOutliner::OnMouseCaptureLost(const FCaptureLostEvent& Event)
{
	Scroll.CaptureLost();
	bEndVisibilityPainting = true;
	SImGuiContextWidget::OnMouseCaptureLost(Event);
}
FReply SImGuiSceneOutliner::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	FReply Reply = SImGuiContextWidget::OnMouseButtonUp(Geometry, Event);
	const bool bPanned = Scroll.MouseUp(Event);
	if (Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (bPanned)
		{
			return FReply::Handled().ReleaseMouseCapture();
		}
		const auto Item = ItemAt(Event.GetScreenSpacePosition());
		if (const TSharedPtr<SSceneOutliner> Outliner = WeakOutliner.Pin())
		{
			const TSharedPtr<SSceneOutlinerTreeView> Tree = Outliner->GetTreeView();
			if (Tree && Item && !Tree->IsItemSelected(Item))
			{
				Outliner->SetCustomTreeSelection({Item}, true, true, ESelectInfo::OnMouseClick);
			}
			ShowContextMenu();
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && !Rename.IsActive() && !Event.IsControlDown() && !Event.IsCommandDown() && !Event.IsShiftDown() && !FSlateApplication::Get().IsDragDropping())
	{
		const auto Outliner = WeakOutliner.Pin();
		for (const FHitItem& Hit : HitItems)
		{
			if (Outliner && Hit.NameRect.ContainsPoint(Event.GetScreenSpacePosition()) && Outliner->GetTreeView()->GetNumItemsSelected() > 1 && Outliner->GetTreeView()->IsItemSelected(Hit.Item))
			{
				PendingActions.Add([Weak = WeakOutliner, Item = Hit.Item]()
				                   {
					if (const auto Backend = Weak.Pin())
					{
						Backend->SetCustomTreeSelection({Item}, true, true, ESelectInfo::OnMouseClick);
						Backend->ClickCustomTreeItem(Item);
					} });
				break;
			}
		}
	}
	return Reply;
}

FReply SImGuiSceneOutliner::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	FReply Reply = SImGuiContextWidget::OnMouseButtonDoubleClick(Geometry, Event);
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && !Rename.IsActive())
	{
		FSceneOutlinerTreeItemPtr Item;
		for (const FHitItem& Hit : HitItems)
		{
			if (Hit.NameRect.ContainsPoint(Event.GetScreenSpacePosition()))
			{
				Item = Hit.Item;
				break;
			}
		}
		if (Item)
		{
			PendingActions.Add([Weak = WeakOutliner, Item]()
			                   {
				if (const auto Outliner = Weak.Pin(); Outliner && Item->IsValid() && Item->CanInteract() && Item->Flags.bInteractive)
				{
					Outliner->SetCustomTreeSelection({Item}, true, true, ESelectInfo::OnMouseClick);
					Outliner->ActivateCustomTreeItem(Item);
				} });
		}
	}
	return Reply;
}

FReply SImGuiSceneOutliner::OnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const auto Outliner = WeakOutliner.Pin();
	const auto Item = DragSourceItem.Pin();
	if (!Outliner || !Item || !Item->IsValid() || !Item->CanInteract() || !Item->Flags.bInteractive || !Outliner->CanSupportDragAndDrop())
	{
		return FReply::Unhandled();
	}
	if (!Outliner->GetTreeView()->IsItemSelected(Item))
	{
		Outliner->SetCustomTreeSelection({Item}, true, true, ESelectInfo::OnMouseClick);
	}
	if (const auto Operation = Outliner->CreateDragDropOperation(Event, Outliner->GetTreeView()->GetSelectedItems()))
	{
		FTBImGuiContextScope Scope(OwningContext);
		if (Scope.IsValid())
		{
			ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
		}
		return FReply::Handled().BeginDragDrop(Operation.ToSharedRef());
	}
	return FReply::Unhandled();
}

FReply SImGuiSceneOutliner::HandleDrop(const FDragDropEvent& Event, bool bApplyDrop)
{
	const auto Outliner = WeakOutliner.Pin();
	const auto Operation = Event.GetOperation();
	if (!Outliner || !Operation || !Outliner->CanSupportDragAndDrop() || !Outliner->GetSharedData().bShowParentTree)
	{
		return FReply::Unhandled();
	}
	FSceneOutlinerDragDropPayload Payload(*Operation);
	if (!Outliner->ParseDragDrop(Payload, *Operation))
	{
		return FReply::Unhandled();
	}
	const auto Item = ItemAt(Event.GetScreenSpacePosition());
	FFolderTreeItem Root{FFolder(Outliner->GetMode()->GetRootObject())};
	ISceneOutlinerTreeItem& Target = Item ? *Item : Root;
	const FSceneOutlinerDragValidationInfo Validation = Outliner->ValidateDrop(Target, Payload);
	if (bApplyDrop)
	{
		if (Validation.IsValid())
		{
			Outliner->OnDropPayload(Target, Payload, Validation);
		}
	}
	else
	{
		if (!Outliner->UpdateOperationDecorator(Event, &Validation) && Operation->IsOfType<FDecoratedDragDropOp>())
		{
			StaticCastSharedPtr<FDecoratedDragDropOp>(Operation)->SetToolTip(Validation.ValidationText,
			                                                                 FAppStyle::GetBrush(Validation.IsValid() ? TEXT("Graph.ConnectorFeedback.OK") : TEXT("Graph.ConnectorFeedback.Error")));
		}
		if (Item)
		{
			Outliner->OnDragOverItem(Event, Target);
		}
	}
	return FReply::Handled();
}

FReply SImGuiSceneOutliner::OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	const auto Item = ItemAt(Event.GetScreenSpacePosition());
	if (DragHoverItem.Pin() != Item)
	{
		DragHoverItem = Item;
		DragHoverStart = FPlatformTime::Seconds();
	}
	return HandleDrop(Event, false);
}

void SImGuiSceneOutliner::OnDragLeave(const FDragDropEvent& Event)
{
	DragHoverItem.Reset();
	if (const auto Outliner = WeakOutliner.Pin(); Outliner && !Outliner->UpdateOperationDecorator(Event, nullptr))
	{
		if (const auto Operation = Event.GetOperationAs<FDecoratedDragDropOp>())
		{
			Operation->ResetToDefaultToolTip();
		}
	}
}

FReply SImGuiSceneOutliner::OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	FReply Reply = HandleDrop(Event, true);
	OnDragLeave(Event);
	return Reply;
}

void SImGuiSceneOutliner::DrawRow(int32 Index)
{
	const auto Outliner = WeakOutliner.Pin();
	const FRow& Row = Rows[Index];
	const auto& Item = Row.Item;
	const bool bInteractive = Item->CanInteract() && Item->Flags.bInteractive;
	const bool bCategory = Item->IsA<FFolderTreeItem>() || Item->IsA<FWorldTreeItem>();
	ImGui::PushID(Item.Get());
	ImGui::TableNextRow(ImGuiTableRowFlags_None, ImGui::GetFrameHeight());
	for (int32 ColumnIndex = 0; ColumnIndex < ColumnIds.Num(); ++ColumnIndex)
	{
		ImGui::TableSetColumnIndex(ColumnIndex);
		ImGui::PushID(ColumnIndex);
		const FName ColumnId = ColumnIds[ColumnIndex];
		if (ColumnId == FSceneOutlinerBuiltInColumnTypes::Gutter())
		{
			const TOptional<bool> Visible = Item->ShouldShowVisibilityState() ? TBIMGUI_SCENE_OUTLINER::GetItemVisibility(*Item) : TOptional<bool>();
			if (Visible.IsSet())
			{
				ImGui::BeginDisabled(!bInteractive);
				ImGui::SmallButton(Visible.GetValue() ? "O" : "-");
				const bool bBeginPaint = ImGui::IsItemActivated();
				if (bBeginPaint)
				{
					bVisibilityPainting = true;
					bVisibilityPaintValue = !Visible.GetValue();
					VisibilityPaintedItems.Reset();
				}
				if (bVisibilityPainting && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !VisibilityPaintedItems.Contains(Item.Get()))
				{
					VisibilityPaintedItems.Add(Item.Get());
					const auto Targets = bBeginPaint && SelectedItems.Contains(Item.Get()) ? Outliner->GetTreeView()->GetSelectedItems() : TArray<FSceneOutlinerTreeItemPtr>{Item};
					PendingActions.Add([Weak = TWeakPtr<SImGuiSceneOutliner>(SharedThis(this)), Targets, bVisible = bVisibilityPaintValue]()
					                   {
						const auto View = Weak.Pin();
						if (!View)
						{
							return;
						}
						if (!View->VisibilityTransaction)
						{
							View->VisibilityTransaction = MakeShared<FScopedTransaction>(NSLOCTEXT("TBImGuiEditor", "OutlinerVisibility", "Change Actor Visibility"));
						}
						for (const auto& Target : Targets)
						{
							if (Target->IsValid() && Target->CanInteract() && Target->Flags.bInteractive)
							{
								TBIMGUI_SCENE_OUTLINER::SetItemVisibility(*Target, bVisible);
							}
						}
						if (const auto Backend = View->WeakOutliner.Pin())
						{
							Backend->Refresh();
						}
						if (GEditor)
						{
							GEditor->RedrawAllViewports();
						} });
				}
				ImGui::EndDisabled();
			}
		}
		else if (ColumnId == FSceneOutlinerBuiltInColumnTypes::Label())
		{
			const float Indent = Row.Depth * ImGui::GetTreeNodeToLabelSpacing();
			ImGui::Indent(Indent);
			ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
			// AK CHANGES - Outliner category headers
			if (bCategory)
			{
				Flags |= ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns;
			}
			if (!Row.bHasChildren)
			{
				Flags |= ImGuiTreeNodeFlags_Leaf;
			}
			if (SelectedItems.Contains(Item.Get()))
			{
				Flags |= ImGuiTreeNodeFlags_Selected;
			}
			const bool bRenaming = ActiveRename.Pin() == Item && Rename.IsActive();
			if (bRenaming)
			{
				ImGui::SetNextItemAllowOverlap();
			}
			ImGui::SetNextItemOpen(Outliner->IsItemExpanded(Item), ImGuiCond_Always);
			ImGui::BeginDisabled(!bInteractive);
			const bool bOpen = ImGui::TreeNodeEx("##Item", Flags, "%s", bRenaming ? "" : TCHAR_TO_UTF8(*Item->GetDisplayString()));
			ImGui::EndDisabled();
			const ImVec2 Min = ImGui::GetItemRectMin();
			const ImVec2 Max = ImGui::GetItemRectMax();
			const ImVec2 ClipMin = ImGui::GetWindowDrawList()->GetClipRectMin();
			const ImVec2 ClipMax = ImGui::GetWindowDrawList()->GetClipRectMax();
			const ImVec2 NamePos(Min.x + ImGui::GetTreeNodeToLabelSpacing(), Min.y + (bCategory ? ImGui::GetStyle().FramePadding.y : 0.0f));
			if (ImGui::IsItemVisible())
			{
				const FVector2f ScreenMin(GetCachedGeometry().LocalToAbsolute(FVector2D(ImGui::GetWindowPos().x, FMath::Max(Min.y, ClipMin.y))));
				const FVector2f ScreenMax(GetCachedGeometry().LocalToAbsolute(FVector2D(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), FMath::Min(Max.y, ClipMax.y))));
				const FVector2f NameMin(GetCachedGeometry().LocalToAbsolute(FVector2D(NamePos.x, Min.y)));
				const FVector2f NameMax(GetCachedGeometry().LocalToAbsolute(FVector2D(Max.x, Max.y)));
				HitItems.Add({Item, FSlateRect(ScreenMin.X, ScreenMin.Y, ScreenMax.X, ScreenMax.Y), FSlateRect(NameMin.X, NameMin.Y, NameMax.X, NameMax.Y)});
			}
			if (Row.bHasChildren && ImGui::IsItemToggledOpen())
			{
				Outliner->SetCustomTreeExpansion(Item, bOpen, ImGui::GetIO().KeyShift);
			}
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && !bRenaming)
			{
				SelectRow(Index, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift);
			}
			if (Outliner->GetCustomTreeScrollItem() == Item)
			{
				if (Min.y >= ClipMin.y && Max.y <= ClipMax.y)
				{
					PendingActions.Add([Weak = WeakOutliner, Item]()
					                   {
						if (const auto Backend = Weak.Pin())
						{
							Backend->CompleteCustomTreeScroll(Item);
						} });
				}
				else
				{
					ImGui::SetScrollHereY();
				}
			}
			if (bRenaming && ImGui::IsItemVisible())
			{
				const ImVec2 Cursor = ImGui::GetCursorScreenPos();
				ImGui::SetCursorScreenPos(NamePos);
				Rename.Draw(Max.x - NamePos.x);
				bRenameDrawn = true;
				ImGui::SetCursorScreenPos(ImVec2(Cursor.x, Cursor.y - ImGui::GetStyle().ItemSpacing.y));
				ImGui::Dummy(ImVec2(0.0f, 0.0f));
			}
			ImGui::Unindent(Indent);
		}
		else if (ColumnId == FSceneOutlinerBuiltInColumnTypes::Pinned())
		{
			if (Item->ShouldShowPinnedState())
			{
				const auto Targets = SelectedItems.Contains(Item.Get()) ? Outliner->GetTreeView()->GetSelectedItems() : TArray<FSceneOutlinerTreeItemPtr>{Item};
				const bool bPinned = TBIMGUI_SCENE_OUTLINER::GetItemPinned(*Item);
				ImGui::BeginDisabled(bPinned ? !Outliner->CanUnpinItems(Targets) : !Outliner->CanPinItems(Targets));
				if (ImGui::SmallButton(bPinned ? "P" : "+"))
				{
					PendingActions.Add([Weak = WeakOutliner, Targets, bPinned]()
					                   {
						if (const auto Backend = Weak.Pin())
						{
							if (bPinned)
							{
								Backend->UnpinItems(Targets);
							}
							else
							{
								Backend->PinItems(Targets);
							}
						} });
				}
				ImGui::EndDisabled();
			}
		}
		else if (const auto Column = Outliner->GetColumns().FindRef(ColumnId); !bCategory && Column)
		{
			TArray<FString> Strings;
			Column->PopulateSearchStrings(*Item, Strings);
			const FString Text = FString::Join(Strings, TEXT(" | "));
			if (ImGui::Selectable(Text.IsEmpty() ? "..." : TCHAR_TO_UTF8(*Text), SelectedItems.Contains(Item.Get()), ImGuiSelectableFlags_AllowDoubleClick))
			{
				SelectRow(Index, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift);
				if (Text.IsEmpty() || ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					PendingActions.Add([Weak = TWeakPtr<SImGuiSceneOutliner>(SharedThis(this)), ColumnId, Item]()
					                   {
						if (const auto View = Weak.Pin())
						{
							View->ShowColumnEditor(ColumnId, Item);
						} });
				}
			}
		}
		ImGui::PopID();
	}
	ImGui::PopID();
}

void SImGuiSceneOutliner::DrawImGuiContent()
{
	const auto Outliner = WeakOutliner.Pin();
	if (!Outliner)
	{
		return;
	}
	if (!bThemeApplied)
	{
		FTBImGuiEditorModule::ApplyThemeToCurrentContext();
		bThemeApplied = true;
	}
	Rows.Reset();
	HitItems.Reset();
	AppendRows(Outliner->GetTreeView()->GetRootItems(), 0);
	SelectedItems.Reset();
	for (const auto& Item : Outliner->GetTreeView()->GetSelectedItems())
	{
		SelectedItems.Add(Item.Get());
	}
	const auto Header = Outliner->GetTreeView()->GetHeaderRow();
	ColumnIds.Reset();
	for (const auto& Column : Header->GetColumns())
	{
		if (Column.bIsVisible && Outliner->GetColumns().Contains(Column.ColumnId))
		{
			ColumnIds.Add(Column.ColumnId);
		}
	}
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->Pos);
	ImGui::SetNextWindowSize(Viewport->Size);
	ImGui::SetNextWindowViewport(Viewport->ID);
	constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin("##WorldOutliner", nullptr, WINDOW_FLAGS))
	{
		if (ImGui::BeginCombo("Columns", "Visible columns"))
		{
			for (const auto& Column : Header->GetColumns())
			{
				const auto* Info = Outliner->GetSharedData().ColumnMap.Find(Column.ColumnId);
				if (!Info || !Info->bCanBeHidden)
				{
					continue;
				}
				bool bVisible = Column.bIsVisible;
				if (ImGui::Checkbox(TCHAR_TO_UTF8(*Column.ColumnId.ToString()), &bVisible))
				{
					PendingActions.Add([Weak = WeakOutliner, Id = Column.ColumnId, bVisible]()
					                   {
						if (const auto Backend = Weak.Pin())
						{
							Backend->SetColumnVisibility(Id, bVisible);
						} });
				}
			}
			ImGui::EndCombo();
		}
		constexpr ImGuiTableFlags TABLE_FLAGS = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
		if (!ColumnIds.IsEmpty() && ImGui::BeginTable("##Hierarchy", ColumnIds.Num(), TABLE_FLAGS, ImVec2(0.0f, -1.0f)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			for (const FName ColumnId : ColumnIds)
			{
				bool bFixed = ColumnId == FSceneOutlinerBuiltInColumnTypes::Gutter() || ColumnId == FSceneOutlinerBuiltInColumnTypes::Pinned();
				float Width = bFixed ? 28.0f : 1.0f;
				if (!bFixed)
				{
					for (const auto& Column : Header->GetColumns())
					{
						if (Column.ColumnId == ColumnId)
						{
							bFixed = Column.SizeRule == EColumnSizeMode::Fixed || Column.SizeRule == EColumnSizeMode::Manual;
							Width = FMath::Max(Column.Width.Get(1.0f), 0.1f);
							break;
						}
					}
				}
				ImGui::TableSetupColumn(TCHAR_TO_UTF8(*ColumnId.ToString()), bFixed ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch, Width);
			}
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
			for (int32 Index = 0; Index < ColumnIds.Num(); ++Index)
			{
				ImGui::TableSetColumnIndex(Index);
				ImGui::PushID(Index);
				const FName ColumnId = ColumnIds[Index];
				for (const auto& Column : Header->GetColumns())
				{
					if (Column.ColumnId != ColumnId)
					{
						continue;
					}
					const FString Label = TBIMGUI_SCENE_OUTLINER::GetColumnLabel(Column);
					ImGui::TableHeader(TCHAR_TO_UTF8(*Label));
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%s", TCHAR_TO_UTF8(*ColumnId.ToString()));
					}
					if (ImGui::IsItemClicked() && Column.OnSortModeChanged.IsBound())
					{
						const EColumnSortMode::Type Mode = Column.SortMode.Get() == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
						PendingActions.Add([Weak = WeakOutliner, Callback = Column.OnSortModeChanged, ColumnId, Mode]()
						                   {
							if (Weak.IsValid())
							{
								Callback.ExecuteIfBound(EColumnSortPriority::Primary, ColumnId, Mode);
							} });
					}
					const bool bStandardHeader = ColumnId == FSceneOutlinerBuiltInColumnTypes::Label() || ColumnId == FSceneOutlinerBuiltInColumnTypes::Gutter() || ColumnId == FSceneOutlinerBuiltInColumnTypes::Pinned() || ColumnId == FSceneOutlinerBuiltInColumnTypes::ActorInfo();
					if (Column.OnGetMenuContent.IsBound() || (!bStandardHeader && Column.HeaderContent.Widget->GetType() != FName(TEXT("SNullWidget"))))
					{
						ImGui::SameLine();
						if (ImGui::SmallButton("..."))
						{
							PendingActions.Add([Weak = TWeakPtr<SImGuiSceneOutliner>(SharedThis(this)), ColumnId]()
							                   {
								if (const auto View = Weak.Pin())
								{
									View->ShowColumnEditor(ColumnId, nullptr);
								} });
						}
					}
					break;
				}
				ImGui::PopID();
			}
			ImGuiListClipper Clipper;
			Clipper.Begin(Rows.Num());
			const auto ScrollItem = Outliner->GetCustomTreeScrollItem();
			const int32 ScrollIndex = Rows.IndexOfByPredicate([&](const FRow& Row)
			                                                  { return Row.Item == ScrollItem; });
			if (ScrollIndex != INDEX_NONE)
			{
				Clipper.IncludeItemByIndex(ScrollIndex);
			}
			while (Clipper.Step())
			{
				for (int32 Index = Clipper.DisplayStart; Index < Clipper.DisplayEnd; ++Index)
				{
					DrawRow(Index);
				}
			}
			if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				PendingActions.Add([Weak = WeakOutliner]()
				                   {
					if (const auto Backend = Weak.Pin())
					{
						Backend->SetCustomTreeSelection({}, false, true, ESelectInfo::OnMouseClick);
					} });
			}
			Scroll.Apply(GetCachedGeometry(), bVisibilityPainting);
			HandleNavigation();
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void SImGuiSceneOutliner::HandleNavigation()
{
	if (Rows.IsEmpty() || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsAnyItemActive())
	{
		return;
	}
	const auto Outliner = WeakOutliner.Pin();
	const auto Navigation = NavigationItem.Pin();
	int32 Current = Rows.IndexOfByPredicate([&](const FRow& Row)
	                                        { return Row.Item == Navigation; });
	if (Current == INDEX_NONE)
	{
		Current = Rows.IndexOfByPredicate([&](const FRow& Row)
		                                  { return SelectedItems.Contains(Row.Item.Get()); });
	}
	if (!SelectionAnchor.IsValid() && Current != INDEX_NONE)
	{
		SelectionAnchor = Rows[Current].Item;
	}
	int32 Next = INDEX_NONE;
	const int32 Page = FMath::Max(1, FMath::FloorToInt(ImGui::GetWindowHeight() / ImGui::GetFrameHeightWithSpacing()));
	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		Next = FMath::Min(Current + 1, Rows.Num() - 1);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		Next = Current == INDEX_NONE ? Rows.Num() - 1 : FMath::Max(0, Current - 1);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Home))
	{
		Next = 0;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_End))
	{
		Next = Rows.Num() - 1;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
	{
		Next = FMath::Clamp(Current + Page, 0, Rows.Num() - 1);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
	{
		Next = FMath::Clamp(Current - Page, 0, Rows.Num() - 1);
	}
	if (Current != INDEX_NONE)
	{
		const FRow& Row = Rows[Current];
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
		{
			if (Row.bHasChildren && Outliner->IsItemExpanded(Row.Item))
			{
				Outliner->SetCustomTreeExpansion(Row.Item, false, ImGui::GetIO().KeyShift);
			}
			else
			{
				const auto Parent = Row.Item->GetParent();
				Next = Rows.IndexOfByPredicate([&](const FRow& Candidate)
				                               { return Candidate.Item == Parent; });
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && Row.bHasChildren)
		{
			if (!Outliner->IsItemExpanded(Row.Item))
			{
				Outliner->SetCustomTreeExpansion(Row.Item, true, ImGui::GetIO().KeyShift);
			}
			else if (Current + 1 < Rows.Num())
			{
				Next = Current + 1;
			}
		}
	}
	if (Next != INDEX_NONE)
	{
		if (!ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift)
		{
			SelectRow(Next, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift, ESelectInfo::OnKeyPress);
		}
		const auto Item = Rows[Next].Item;
		NavigationItem = Item;
		PendingActions.Add([Weak = WeakOutliner, Item]()
		                   { if (const auto Backend = Weak.Pin()) { Backend->ScrollItemIntoView(Item); } });
	}
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A) && Outliner->GetMode()->GetSelectionMode() == ESelectionMode::Multi)
	{
		TArray<FSceneOutlinerTreeItemPtr> Items;
		for (const auto& Row : Rows)
		{
			if (Row.Item->CanInteract() && Row.Item->Flags.bInteractive)
			{
				Items.Add(Row.Item);
			}
		}
		PendingActions.Add([Weak = WeakOutliner, Items = MoveTemp(Items)]()
		                   {
			if (const auto Backend = Weak.Pin())
			{
				Backend->SetCustomTreeSelection(Items, true, true, ESelectInfo::OnKeyPress);
			} });
	}
}
