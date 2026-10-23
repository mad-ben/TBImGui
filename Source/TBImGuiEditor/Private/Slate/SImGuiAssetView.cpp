// Copyright Lewis Scrivens. All Rights Reserved.
#include "Slate/SImGuiAssetView.h"

#include "ContentBrowserItem.h"
#include "Framework/Application/SlateApplication.h"

#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "Editor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Layout/WidgetPath.h"
#include "SAssetView.h"
#include "TBImGuiAssetThumbnails.h"
#include "TBImGuiEditor.h"
#include "Widgets/Layout/SBox.h"
#include "TBImGuiEditorSettings.h"
#include "AssetRegistry/AssetData.h"
#include "Misc/PackageName.h"

// AK CHANGES - Asset captions
namespace TBIMGUI_ASSET_CAPTIONS
{
static FString GetCaption(const FContentBrowserItem& Item)
{
	FAssetData Asset;
	if (!Item.Legacy_TryGetAssetData(Asset))
	{
		return FString();
	}
	FString Value;
	if (Asset.GetTagValue(TEXT("Parent"), Value) && !Value.IsEmpty() && Value != TEXT("None"))
	{
		const FString ParentPath = FPackageName::ExportTextPathToObjectPath(Value);
		return FString::Printf(TEXT("Parent: %s"), *FPackageName::ObjectPathToObjectName(ParentPath));
	}
	if (Asset.GetTagValue(TEXT("Dimensions"), Value) && !Value.IsEmpty())
	{
		return Value;
	}
	return FString();
}
} // namespace TBIMGUI_ASSET_CAPTIONS

void SImGuiAssetView::Construct(const FArguments& Args)
{
	static uint32 NextContextId = 0;
	FTBImGuiContextParams Params(FString::Printf(TEXT("Assets%08x"), ++NextContextId));
	Params.ViewportMode = ETBImGuiViewportMode::Single;
	Params.bDockingSupport = false;
	SImGuiContextWidget::Construct(SImGuiContextWidget::FArguments().ContextParams(Params));
	Thumbnails = MakeUnique<FTBImGuiAssetThumbnails>();
	VisibleColumns.Add(FAssetViewSortManager::NameColumnId);
	VisibleColumns.Add(FAssetViewSortManager::ClassColumnId);
	VisibleColumns.Add(FAssetViewSortManager::PathColumnId);
}

SImGuiAssetView::~SImGuiAssetView() = default;

TSharedRef<SWidget> SImGuiAssetView::CreateView(TArray<TSharedPtr<FAssetViewItem>>* Source)
{
	OnItemListChanged(Source);
	return SharedThis(this);
}

void SImGuiAssetView::OnItemListChanged(TArray<TSharedPtr<FAssetViewItem>>* Source)
{
	ItemsSource = Source;
	bItemsDirty = true;
	bGridLabelsDirty = true;
}

void SImGuiAssetView::RequestScrollIntoView(const TSharedPtr<FAssetViewItem>& Item)
{
	PendingScroll = Item;
	bItemsDirty = true;
	bGridLabelsDirty = true;
}

TSharedPtr<SAssetView> SImGuiAssetView::FindOwner()
{
	if (const auto Pinned = Owner.Pin())
	{
		return Pinned;
	}
	for (auto Parent = GetParentWidget(); Parent; Parent = Parent->GetParentWidget())
	{
		if (Parent->GetType() == FName(TEXT("SAssetView")))
		{
			const auto Pinned = StaticCastSharedPtr<SAssetView>(Parent);
			Owner = Pinned;
			return Pinned;
		}
	}
	return nullptr;
}

void SImGuiAssetView::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
	const auto Backend = FindOwner();
	if (!Backend)
	{
		return;
	}
	if (!bSettingsLoaded && GConfig && !GEditorPerProjectIni.IsEmpty())
	{
		SettingsSection = FString::Printf(TEXT("TBImGui.ContentBrowser.%s"), *Backend->GetCustomViewInstanceName().ToString());
		GConfig->GetBool(*SettingsSection, TEXT("Grid"), bGrid, GEditorPerProjectIni);
		TArray<FString> SavedColumns;
		if (GConfig->GetArray(*SettingsSection, TEXT("Columns"), SavedColumns, GEditorPerProjectIni))
		{
			VisibleColumns.Reset();
			for (const FString& Column : SavedColumns)
			{
				VisibleColumns.Add(FName(*Column));
			}
			VisibleColumns.Add(FAssetViewSortManager::NameColumnId);
		}
		bSettingsLoaded = true;
	}
	const float ConfiguredTileSize = FMath::Clamp(UTBImGuiEditorSettings::Get()->ContentBrowserThumbnailSize, 32.0f, 256.0f);
	if (TileSize != ConfiguredTileSize)
	{
		TileSize = ConfiguredTileSize;
		bGridLabelsDirty = true;
	}
	if (const auto Item = DragHoverItem.Pin(); Item && Item->GetItem().IsFolder() && FSlateApplication::Get().IsDragDropping() && CurrentTime - DragHoverStart > 0.75)
	{
		DragHoverItem.Reset();
		DoubleClicked.ExecuteIfBound(Item);
	}
	if (const auto Item = ActiveRename.Pin(); Item && (!Rename.IsActive() || !Backend->IsRenamingAsset()))
	{
		Rename.Cancel();
		Item->OnRenameRequested().BindSP(this, &SImGuiAssetView::ShowRenameEditor, TWeakPtr<FAssetViewItem>(Item));
		ActiveRename.Reset();
	}
	SetContentSize(Geometry.GetLocalSize());
	Thumbnails->Tick();
	if (bItemsDirty)
	{
		RebuildItems();
	}
	bRenameDrawn = false;
	SImGuiContextWidget::Tick(Geometry, CurrentTime, DeltaTime);
	if (bSaveThumbnailSize)
	{
		GetMutableDefault<UTBImGuiEditorSettings>()->SaveConfig();
		bSaveThumbnailSize = false;
	}
	if (Rename.IsActive() && !bRenameDrawn)
	{
		Rename.Cancel();
	}
	Rename.Flush();
	TArray<TFunction<void()>> Actions = MoveTemp(PendingActions);
	PendingActions.Reset();
	for (auto& Action : Actions)
	{
		Action();
	}
}

void SImGuiAssetView::RebuildItems()
{
	Items = ItemsSource ? *ItemsSource : TArray<TSharedPtr<FAssetViewItem>>();
	Items.RemoveAll([](const auto& Item)
	                { return !Item || !Item->GetItem().IsValid(); });
	bItemsDirty = false;
	if (const auto Backend = FindOwner())
	{
		Backend->GetCustomViewColumns(AvailableColumns);
	}
	TSet<const FAssetViewItem*> CurrentItems;
	for (const auto& Item : Items)
	{
		CurrentItems.Add(Item.Get());
		if (ActiveRename.Pin() != Item || !Rename.IsActive())
		{
			Item->OnRenameRequested().BindSP(this, &SImGuiAssetView::ShowRenameEditor, TWeakPtr<FAssetViewItem>(Item));
		}
	}
	const int32 OldCount = Selected.Num();
	Selected.RemoveAll([&](const auto& Item)
	                   {
		const FName* Path = SelectedPaths.Find(Item.Get());
		return !CurrentItems.Contains(Item.Get()) || !Path || *Path != Item->GetItem().GetVirtualPath(); });
	SelectedPaths.Reset();
	for (const auto& Item : Selected)
	{
		SelectedPaths.Add(Item.Get(), Item->GetItem().GetVirtualPath());
	}
	if (OldCount != Selected.Num())
	{
		PendingActions.Add([Weak = TWeakPtr<SImGuiAssetView>(SharedThis(this))]()
		                   {
			if (const auto View = Weak.Pin()) { View->SelectionChanged.ExecuteIfBound(nullptr, ESelectInfo::Direct); } });
	}
}

void SImGuiAssetView::AssignSelected(const TSharedPtr<FAssetViewItem>& Item, bool bSelected)
{
	if (!Item || !Item->GetItem().IsValid())
	{
		return;
	}
	if (bSelected)
	{
		Selected.AddUnique(Item);
		SelectedPaths.Add(Item.Get(), Item->GetItem().GetVirtualPath());
	}
	else
	{
		Selected.Remove(Item);
		SelectedPaths.Remove(Item.Get());
	}
}

void SImGuiAssetView::SetSelection(const TSharedPtr<FAssetViewItem>& Item, bool bSelected, ESelectInfo::Type Info)
{
	const bool bWasSelected = Selected.Contains(Item);
	AssignSelected(Item, bSelected);
	if (bSelected && Item)
	{
		NavigationItem = Item;
		Anchor = Item;
	}
	if (bWasSelected != Selected.Contains(Item))
	{
		SelectionChanged.ExecuteIfBound(Item, Info);
	}
}

void SImGuiAssetView::ClearSelection()
{
	if (!Selected.IsEmpty())
	{
		Selected.Reset();
		SelectedPaths.Reset();
		SelectionChanged.ExecuteIfBound(nullptr, ESelectInfo::Direct);
	}
}

void SImGuiAssetView::FocusList()
{
	FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::SetDirectly);
}

FText SImGuiAssetView::GetViewDisplayName()
{
	return NSLOCTEXT("TBImGuiEditor", "AssetViewName", "ImGui");
}
FText SImGuiAssetView::GetViewTooltipText()
{
	return NSLOCTEXT("TBImGuiEditor", "AssetViewTooltip", "Browse assets with the ImGui grid or list.");
}

void SImGuiAssetView::SelectIndex(int32 Index, bool bToggle, bool bRange, ESelectInfo::Type Info)
{
	if (!Items.IsValidIndex(Index))
	{
		return;
	}
	const auto Backend = FindOwner();
	const ESelectionMode::Type SelectionMode = Backend ? Backend->GetCustomViewSelectionMode() : ESelectionMode::None;
	if (SelectionMode == ESelectionMode::None)
	{
		return;
	}
	bToggle &= SelectionMode == ESelectionMode::Multi;
	bRange &= SelectionMode == ESelectionMode::Multi;
	const auto Item = Items[Index];
	TArray<TSharedPtr<FAssetViewItem>> Targets;
	if (bRange)
	{
		const int32 Start = Items.IndexOfByKey(Anchor.Pin());
		if (Start != INDEX_NONE)
		{
			for (int32 Row = FMath::Min(Start, Index); Row <= FMath::Max(Start, Index); ++Row)
			{
				Targets.Add(Items[Row]);
			}
		}
	}
	if (Targets.IsEmpty())
	{
		Targets.Add(Item);
	}
	const bool bDeselect = (bToggle || SelectionMode == ESelectionMode::SingleToggle) && !bRange && Selected.Contains(Item);
	NavigationItem = Item;
	if (!bRange)
	{
		Anchor = Item;
	}
	PendingActions.Add([Weak = TWeakPtr<SImGuiAssetView>(SharedThis(this)), Targets = MoveTemp(Targets), Item, Info, bToggle, bDeselect]()
	                   {
		if (const auto View = Weak.Pin())
		{
			if (!bToggle) { View->Selected.Reset(); View->SelectedPaths.Reset(); }
			for (const auto& Target : Targets) { View->AssignSelected(Target, !bDeselect); }
			View->SelectionChanged.ExecuteIfBound(Item, Info);
		} });
}

void SImGuiAssetView::ShowContextMenu()
{
	if (!ContextMenuOpened.IsBound())
	{
		return;
	}
	if (const auto Menu = ContextMenuOpened.Execute())
	{
		FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Menu.ToSharedRef(),
		                                  FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
	}
}

void SImGuiAssetView::ShowRenameEditor(TWeakPtr<FAssetViewItem> WeakItem)
{
	Rename.Cancel();
	const auto Item = WeakItem.Pin();
	const auto Backend = FindOwner();
	if (!Backend || !Item || !Item->GetItem().IsValid())
	{
		return;
	}
	FSlateRect RenameAnchor = GetCachedGeometry().GetLayoutBoundingRect();
	for (const FHitItem& Hit : HitItems)
	{
		if (Hit.Item.Pin() == Item)
		{
			RenameAnchor = Hit.NameRect;
			break;
		}
	}
	if (Rename.Begin(Backend->CreateCustomViewRenameEditor(Item, RenameAnchor)))
	{
		ActiveRename = Item;
		FocusList();
	}
	Item->OnRenameRequested().BindSP(this, &SImGuiAssetView::ShowRenameEditor, WeakItem);
}

void SImGuiAssetView::ProcessItem(int32 Index, bool bClicked, const FSlateRect& NameRect)
{
	const auto Item = Items[Index];
	const ImVec2 Min = ImGui::GetItemRectMin();
	const ImVec2 Max = ImGui::GetItemRectMax();
	const ImVec2 ClipMin = ImGui::GetWindowDrawList()->GetClipRectMin();
	const ImVec2 ClipMax = ImGui::GetWindowDrawList()->GetClipRectMax();
	if (ImGui::IsItemVisible())
	{
		const FVector2f ScreenMin(GetCachedGeometry().LocalToAbsolute(FVector2D(Min.x, FMath::Max(Min.y, ClipMin.y))));
		const FVector2f ScreenMax(GetCachedGeometry().LocalToAbsolute(FVector2D(Max.x, FMath::Min(Max.y, ClipMax.y))));
		const float VisibleTop = FMath::Max(Min.y, ClipMin.y);
		const float VisibleBottom = FMath::Min(Max.y, ClipMax.y);
		const float NameTop = FMath::Clamp(NameRect.Top, VisibleTop, FMath::Max(VisibleTop, VisibleBottom - NameRect.GetSize().Y));
		const FVector2f NameMin(GetCachedGeometry().LocalToAbsolute(FVector2D(NameRect.Left, NameTop)));
		const FVector2f NameMax(GetCachedGeometry().LocalToAbsolute(FVector2D(NameRect.Right, NameTop + NameRect.GetSize().Y)));
		HitItems.Add({Item, FSlateRect(ScreenMin.X, ScreenMin.Y, ScreenMax.X, ScreenMax.Y), FSlateRect(NameMin.X, NameMin.Y, NameMax.X, NameMax.Y)});
	}
	if (bClicked)
	{
		SelectIndex(Index, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift);
	}
	if (PendingScroll.Pin() == Item)
	{
		if (ImGui::IsItemVisible() && NameRect.Top >= ClipMin.y && NameRect.Bottom <= ClipMax.y)
		{
			PendingScroll.Reset();
			PendingActions.Add([Weak = TWeakPtr<SImGuiAssetView>(SharedThis(this)), Item]()
			                   {
				if (const auto View = Weak.Pin())
				{
					View->ScrolledIntoView.ExecuteIfBound(Item, nullptr);
				} });
		}
		else
		{
			ImGui::SetScrollFromPosY((NameRect.Top + NameRect.Bottom) * 0.5f - ImGui::GetWindowPos().y, 0.5f);
		}
	}
}

void SImGuiAssetView::DrawItem(int32 Index, bool bTile)
{
	const auto& Item = Items[Index];
	ImGui::PushID(Item.Get());
	const float LineHeight = ImGui::GetTextLineHeight();
	const float Height = bTile ? TileSize + GridLabelHeight + 2.0f * LineHeight + 29.0f : 36.0f;
	const ImVec2 Start = ImGui::GetCursorScreenPos();
	const bool bRenaming = ActiveRename.Pin() == Item && Rename.IsActive();
	if (bRenaming)
	{
		ImGui::SetNextItemAllowOverlap();
	}
	const ImGuiSelectableFlags Flags = bTile ? ImGuiSelectableFlags_None : ImGuiSelectableFlags_SpanAllColumns;
	const bool bClicked = ImGui::Selectable("##Asset", Selected.Contains(Item), Flags, ImVec2(0.0f, Height));
	const ImVec2 End = ImGui::GetItemRectMax();
	const float ImageSize = bTile ? FMath::Max(16.0f, FMath::Min(TileSize, End.x - Start.x - 16.0f)) : 30.0f;
	const ImVec2 ImageMin(Start.x + 6.0f, Start.y + 3.0f);
	const ImVec2 ImageMax(ImageMin.x + ImageSize, ImageMin.y + ImageSize);
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	const bool bFramedThumbnail = bTile && Item->GetItem().IsFile();
	if (bFramedThumbnail)
	{
		DrawList->AddRectFilled(ImVec2(ImageMin.x + 3.0f, ImageMin.y + 3.0f), ImVec2(ImageMax.x + 3.0f, ImageMax.y + 3.0f), IM_COL32(0, 0, 0, 35));
		DrawList->AddRectFilled(ImVec2(ImageMin.x + 2.0f, ImageMin.y + 2.0f), ImVec2(ImageMax.x + 2.0f, ImageMax.y + 2.0f), IM_COL32(0, 0, 0, 70));
		DrawList->AddRectFilled(ImVec2(ImageMin.x - 1.0f, ImageMin.y - 1.0f), ImVec2(ImageMax.x + 1.0f, ImageMax.y + 1.0f), IM_COL32(0, 0, 0, 220));
	}
	if (UTexture* Texture = Thumbnails->FindOrQueue(Item->GetItem()))
	{
		DrawList->AddImage(Texture, ImageMin, ImageMax);
	}
	else
	{
		// Brief loading background while the native widget is rendered into its cached texture.
		DrawList->AddRectFilled(ImageMin, ImageMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
	}
	if (bFramedThumbnail)
	{
		const FColor TypeColor = Thumbnails->GetTypeColor(Item->GetItem()).ToFColorSRGB();
		const ImU32 Accent = IM_COL32(TypeColor.R, TypeColor.G, TypeColor.B, 210);
		DrawList->AddRectFilled(ImageMin, ImVec2(ImageMax.x, ImageMin.y + 1.0f), Accent);
		DrawList->AddRectFilled(ImageMin, ImVec2(ImageMin.x + 1.0f, ImageMax.y), Accent);
	}
	const ImVec2 LabelPos = bTile ? ImVec2(Start.x + 6.0f, ImageMax.y + 7.0f) : ImVec2(ImageMax.x + 8.0f, Start.y + 10.0f);
	// Clip labels to their own cell; a selectable list row spans the other columns.
	const float CellRight = bTile ? End.x - 4.0f : Start.x + ImGui::GetContentRegionAvail().x;
	DrawList->PushClipRect(Start, ImVec2(CellRight, End.y), true);
	if (bTile)
	{
		if (!bRenaming)
		{
			DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), LabelPos, ImGui::GetColorU32(ImGuiCol_Text),
			                  TCHAR_TO_UTF8(*Item->GetItem().GetDisplayName().ToString()), nullptr, GridWrapWidth);
		}
		DrawList->AddText(ImVec2(LabelPos.x, LabelPos.y + GridLabelHeight + 3.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled),
		                  TCHAR_TO_UTF8(*Item->GetAssetClassText().ToString()));
		const FString Caption = TBIMGUI_ASSET_CAPTIONS::GetCaption(Item->GetItem());
		if (!Caption.IsEmpty())
		{
			DrawList->AddText(ImVec2(LabelPos.x, LabelPos.y + GridLabelHeight + LineHeight + 6.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled),
			                  TCHAR_TO_UTF8(*Caption));
		}
	}
	else if (!bRenaming)
	{
		DrawList->AddText(LabelPos, ImGui::GetColorU32(ImGuiCol_Text), TCHAR_TO_UTF8(*Item->GetItem().GetDisplayName().ToString()));
	}
	DrawList->PopClipRect();
	const bool bVisible = ImGui::IsItemVisible();
	ProcessItem(Index, bClicked && !bRenaming, FSlateRect(LabelPos.x, LabelPos.y, CellRight, LabelPos.y + LineHeight));
	if (bRenaming && bVisible)
	{
		const ImVec2 Cursor = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(LabelPos);
		Rename.Draw(CellRight - LabelPos.x);
		bRenameDrawn = true;
		ImGui::SetCursorScreenPos(ImVec2(Cursor.x, Cursor.y - ImGui::GetStyle().ItemSpacing.y));
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
	}
	ImGui::PopID();
}

void SImGuiAssetView::DrawList()
{
	const auto Backend = FindOwner();
	if (!Backend)
	{
		return;
	}
	TArray<TPair<FName, FText>> Columns;
	for (const auto& Column : AvailableColumns)
	{
		if (VisibleColumns.Contains(Column.Key))
		{
			Columns.Add(Column);
		}
	}
	constexpr ImGuiTableFlags TABLE_FLAGS = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
	if (!Columns.IsEmpty() && ImGui::BeginTable("##AssetList", Columns.Num(), TABLE_FLAGS, ImVec2(0.0f, -1.0f)))
	{
		ImGui::TableSetupScrollFreeze(1, 1);
		for (const auto& Column : Columns)
		{
			ImGui::TableSetupColumn(TCHAR_TO_UTF8(*Column.Value.ToString()), ImGuiTableColumnFlags_WidthStretch, 1.0f);
		}
		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		const auto Sort = Backend->GetSortManager().Pin();
		for (int32 Index = 0; Index < Columns.Num(); ++Index)
		{
			ImGui::TableSetColumnIndex(Index);
			ImGui::TableHeader(TCHAR_TO_UTF8(*Columns[Index].Value.ToString()));
			if (ImGui::IsItemClicked() && Sort)
			{
				const EColumnSortPriority::Type Priority = ImGui::GetIO().KeyShift ? EColumnSortPriority::Secondary : EColumnSortPriority::Primary;
				const FName ColumnId = Columns[Index].Key;
				const EColumnSortMode::Type Mode = Sort->GetSortColumnId(Priority) == ColumnId && Sort->GetSortMode(Priority) == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
				PendingActions.Add([Weak = Owner, ColumnId, Priority, Mode]()
				                   {
					if (const auto View = Weak.Pin())
					{
						View->SetSortParameters(Priority, ColumnId, Mode);
					} });
			}
		}
		ImGuiListClipper Clipper;
		Clipper.Begin(Items.Num());
		const int32 ScrollIndex = Items.IndexOfByKey(PendingScroll.Pin());
		if (ScrollIndex != INDEX_NONE)
		{
			Clipper.IncludeItemByIndex(ScrollIndex);
		}
		while (Clipper.Step())
		{
			for (int32 Index = Clipper.DisplayStart; Index < Clipper.DisplayEnd; ++Index)
			{
				ImGui::TableNextRow();
				for (int32 ColumnIndex = 0; ColumnIndex < Columns.Num(); ++ColumnIndex)
				{
					ImGui::TableSetColumnIndex(ColumnIndex);
					if (Columns[ColumnIndex].Key == FAssetViewSortManager::NameColumnId)
					{
						DrawItem(Index, false);
					}
					else
					{
						ImGui::TextUnformatted(TCHAR_TO_UTF8(*Backend->GetCustomViewColumnText(Items[Index], Columns[ColumnIndex].Key).ToString()));
					}
				}
			}
		}
		HandleNavigation();
		HandleBackgroundClick();
		Scroll.Apply(GetCachedGeometry());
		ImGui::EndTable();
	}
}

void SImGuiAssetView::DrawGrid()
{
	GridColumns = FMath::Clamp(FMath::FloorToInt(ImGui::GetContentRegionAvail().x / (TileSize + 20.0f)), 1, 32);
	if (ImGui::BeginTable("##AssetGrid", GridColumns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY, ImVec2(0.0f, -1.0f)))
	{
		// Use a shared row height so clipping stays correct, reserving enough space for every wrapped name.
		const float WrapWidth = FMath::Max(16.0f, FMath::FloorToFloat(
		                                              (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize) / GridColumns - 2.0f * ImGui::GetStyle().CellPadding.x - 16.0f));
		if (bGridLabelsDirty || GridWrapWidth != WrapWidth || GridFontSize != ImGui::GetFontSize())
		{
			GridWrapWidth = WrapWidth;
			GridFontSize = ImGui::GetFontSize();
			GridLabelHeight = ImGui::GetTextLineHeight();
			for (const auto& Item : Items)
			{
				GridLabelHeight = FMath::Max(GridLabelHeight,
				                             ImGui::CalcTextSize(TCHAR_TO_UTF8(*Item->GetItem().GetDisplayName().ToString()), nullptr, false, GridWrapWidth).y);
			}
			bGridLabelsDirty = false;
		}
		const int32 RowCount = FMath::DivideAndRoundUp(Items.Num(), GridColumns);
		ImGuiListClipper Clipper;
		Clipper.Begin(RowCount);
		const int32 ScrollIndex = Items.IndexOfByKey(PendingScroll.Pin());
		if (ScrollIndex != INDEX_NONE)
		{
			Clipper.IncludeItemByIndex(ScrollIndex / GridColumns);
		}
		while (Clipper.Step())
		{
			for (int32 Row = Clipper.DisplayStart; Row < Clipper.DisplayEnd; ++Row)
			{
				ImGui::TableNextRow();
				for (int32 Column = 0; Column < GridColumns; ++Column)
				{
					ImGui::TableSetColumnIndex(Column);
					const int32 Index = Row * GridColumns + Column;
					if (Items.IsValidIndex(Index))
					{
						DrawItem(Index, true);
					}
				}
			}
		}
		HandleNavigation();
		HandleBackgroundClick();
		Scroll.Apply(GetCachedGeometry());
		ImGui::EndTable();
	}
}

void SImGuiAssetView::HandleNavigation()
{
	if (Items.IsEmpty() || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsAnyItemActive())
	{
		return;
	}
	int32 Current = Items.IndexOfByKey(NavigationItem.Pin());
	if (Current == INDEX_NONE && !Selected.IsEmpty())
	{
		Current = Items.IndexOfByKey(Selected.Last());
	}
	if (!Anchor.IsValid() && Current != INDEX_NONE)
	{
		Anchor = Items[Current];
	}
	int32 Next = INDEX_NONE;
	const int32 Stride = bGrid ? GridColumns : 1;
	const float RowHeight = bGrid ? TileSize + GridLabelHeight + ImGui::GetTextLineHeight() + 26.0f : 36.0f;
	const int32 PageStride = FMath::Max(1, FMath::FloorToInt(ImGui::GetWindowHeight() / RowHeight)) * Stride;
	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		Next = Current == INDEX_NONE ? 0 : FMath::Min(Current + Stride, Items.Num() - 1);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		Next = Current == INDEX_NONE ? Items.Num() - 1 : FMath::Max(0, Current - Stride);
	}
	if (bGrid && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		Next = FMath::Min(Current + 1, Items.Num() - 1);
	}
	if (bGrid && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		Next = FMath::Max(0, Current - 1);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Home))
	{
		Next = 0;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_End))
	{
		Next = Items.Num() - 1;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
	{
		Next = FMath::Clamp(Current + PageStride, 0, Items.Num() - 1);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
	{
		Next = FMath::Clamp(Current - PageStride, 0, Items.Num() - 1);
	}
	if (Next != INDEX_NONE)
	{
		if (!ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift)
		{
			SelectIndex(Next, ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift, ESelectInfo::OnKeyPress);
		}
		NavigationItem = Items[Next];
		PendingScroll = Items[Next];
	}
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
	{
		PendingActions.Add([Weak = TWeakPtr<SImGuiAssetView>(SharedThis(this))]()
		                   {
			if (const auto View = Weak.Pin())
			{
				const auto Backend = View->FindOwner();
				if (!Backend || Backend->GetCustomViewSelectionMode() != ESelectionMode::Multi)
				{
					return;
				}
				View->Selected.Reset(); View->SelectedPaths.Reset();
				for (const auto& Item : View->Items) { View->AssignSelected(Item, true); }
				View->SelectionChanged.ExecuteIfBound(View->Selected.IsEmpty() ? nullptr : View->Selected.Last(), ESelectInfo::OnKeyPress);
			} });
	}
}

void SImGuiAssetView::HandleBackgroundClick()
{
	if (!ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered())
	{
		return;
	}
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		PendingActions.Add([Weak = TWeakPtr<SImGuiAssetView>(SharedThis(this))]()
		                   {
			if (const auto View = Weak.Pin())
			{
				View->ClearSelection();
			} });
	}
}

bool SImGuiAssetView::IsRightClickScrolling()
{
	return Scroll.IsPanning();
}

void SImGuiAssetView::SaveLayout()
{
	if (!GConfig || SettingsSection.IsEmpty())
	{
		return;
	}
	GConfig->SetBool(*SettingsSection, TEXT("Grid"), bGrid, GEditorPerProjectIni);
	TArray<FString> Columns;
	for (const FName Id : VisibleColumns)
	{
		Columns.Add(Id.ToString());
	}
	GConfig->SetArray(*SettingsSection, TEXT("Columns"), Columns, GEditorPerProjectIni);
}

void SImGuiAssetView::DrawSortControls()
{
	const auto Backend = FindOwner();
	const auto Sort = Backend ? Backend->GetSortManager().Pin() : nullptr;
	if (!Sort)
	{
		return;
	}
	const FName Current = Sort->GetSortColumnId(EColumnSortPriority::Primary);
	const auto* CurrentColumn = AvailableColumns.FindByPredicate([Current](const auto& Column)
	                                                             { return Column.Key == Current; });
	const FString Label = CurrentColumn ? CurrentColumn->Value.ToString() : Current.ToString();
	ImGui::SetNextItemWidth(130.0f);
	if (ImGui::BeginCombo("Sort by", TCHAR_TO_UTF8(*Label)))
	{
		for (const auto& Column : AvailableColumns)
		{
			if (ImGui::Selectable(TCHAR_TO_UTF8(*Column.Value.ToString()), Column.Key == Current))
			{
				PendingActions.Add([Weak = Owner, Id = Column.Key]()
				                   {
					if (const auto View = Weak.Pin())
					{
						View->SetSortParameters(EColumnSortPriority::Primary, Id, TOptional<const EColumnSortMode::Type>());
					} });
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	const bool bDescending = Sort->GetSortMode(EColumnSortPriority::Primary) == EColumnSortMode::Descending;
	if (ImGui::Button(bDescending ? "Descending" : "Ascending"))
	{
		PendingActions.Add([Weak = Owner, bDescending]()
		                   {
			if (const auto View = Weak.Pin())
			{
				View->SetSortParameters(EColumnSortPriority::Primary, TOptional<const FName>(), bDescending ? EColumnSortMode::Ascending : EColumnSortMode::Descending);
			} });
	}
	if (!bGrid)
	{
		if (ImGui::BeginCombo("Columns", "Visible columns"))
		{
			for (const auto& Column : AvailableColumns)
			{
				bool bVisible = VisibleColumns.Contains(Column.Key);
				ImGui::BeginDisabled(Column.Key == FAssetViewSortManager::NameColumnId);
				if (ImGui::Checkbox(TCHAR_TO_UTF8(*Column.Value.ToString()), &bVisible))
				{
					if (bVisible)
					{
						VisibleColumns.Add(Column.Key);
					}
					else
					{
						VisibleColumns.Remove(Column.Key);
					}
					SaveLayout();
				}
				ImGui::EndDisabled();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Copy CSV"))
		{
			TArray<FName> Columns;
			for (const auto& Column : AvailableColumns)
			{
				if (VisibleColumns.Contains(Column.Key))
				{
					Columns.Add(Column.Key);
				}
			}
			PendingActions.Add([Weak = Owner, Columns]()
			                   {
				if (const auto View = Weak.Pin())
				{
					FPlatformApplicationMisc::ClipboardCopy(*View->ExportCustomViewColumns(Columns));
				} });
		}
	}
}

void SImGuiAssetView::DrawImGuiContent()
{
	if (!bThemeApplied)
	{
		FTBImGuiEditorModule::ApplyThemeToCurrentContext();
		bThemeApplied = true;
	}
	HitItems.Reset();
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->Pos);
	ImGui::SetNextWindowSize(Viewport->Size);
	ImGui::SetNextWindowViewport(Viewport->ID);
	constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin("##ContentBrowser", nullptr, Flags))
	{
		if (ImGui::RadioButton("Grid", bGrid))
		{
			bGrid = true;
			SaveLayout();
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("List", !bGrid))
		{
			bGrid = false;
			SaveLayout();
		}
		if (bGrid)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(140.0f);
			if (ImGui::SliderFloat("Thumbnail Size", &TileSize, 32.0f, 256.0f, "%.0f px"))
			{
				bGridLabelsDirty = true;
				GetMutableDefault<UTBImGuiEditorSettings>()->ContentBrowserThumbnailSize = TileSize;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				bSaveThumbnailSize = true;
			}
		}
		DrawSortControls();
		ImGui::Separator();
		if (bGrid)
		{
			DrawGrid();
		}
		else
		{
			DrawList();
		}
	}
	ImGui::End();
}

TSharedPtr<FAssetViewItem> SImGuiAssetView::ItemAt(FVector2D Position) const
{
	for (const auto& Hit : HitItems)
	{
		if (Hit.Rect.ContainsPoint(Position))
		{
			return Hit.Item.Pin();
		}
	}
	return nullptr;
}

void SImGuiAssetView::ShowThumbnailEditor(const TSharedPtr<FAssetViewItem>& Item)
{
	const auto Backend = FindOwner();
	const auto Thumbnail = Thumbnails->FindThumbnail(Item->GetItem());
	if (Backend && Thumbnail)
	{
		FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Backend->CreateCustomViewThumbnailEditor(Thumbnail),
		                                  FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
	}
}

TSharedPtr<IToolTip> SImGuiAssetView::GetToolTip()
{
	const auto Item = ItemAt(FSlateApplication::Get().GetCursorPos());
	const auto Backend = FindOwner();
	if (!Item || !Backend || Rename.IsActive() || Scroll.IsPanning())
	{
		return nullptr;
	}
	if (ToolTipItem.Pin() != Item || !ToolTipSource)
	{
		ToolTipSource = Backend->CreateCustomViewToolTipSource(Item);
		ToolTipItem = Item;
		Item->OnRenameRequested().BindSP(this, &SImGuiAssetView::ShowRenameEditor, TWeakPtr<FAssetViewItem>(Item));
	}
	return ToolTipSource->GetToolTip();
}

bool SImGuiAssetView::OnVisualizeTooltip(const TSharedPtr<SWidget>& Content)
{
	return ToolTipSource && ToolTipSource->OnVisualizeTooltip(Content);
}

void SImGuiAssetView::OnToolTipClosing()
{
	if (ToolTipSource)
	{
		ToolTipSource->OnToolTipClosing();
	}
	ToolTipSource.Reset();
	ToolTipItem.Reset();
}
FReply SImGuiAssetView::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	FReply Reply = SImGuiContextWidget::OnMouseButtonDown(Geometry, Event);
	if (Reply.IsEventHandled())
	{
		Scroll.MouseDown(Event);
		Reply.SetUserFocus(AsShared(), EFocusCause::Mouse);
		DragSourceItem = ItemAt(Event.GetScreenSpacePosition());
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton && DragSourceItem.IsValid() && !Rename.IsActive())
		{
			Reply.DetectDrag(AsShared(), EKeys::LeftMouseButton);
		}
	}
	return Reply;
}

FReply SImGuiAssetView::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	FReply Reply = SImGuiContextWidget::OnMouseMove(Geometry, Event);
	return Scroll.MouseMove(Geometry, Event) ? FReply::Handled() : Reply;
}

void SImGuiAssetView::OnMouseCaptureLost(const FCaptureLostEvent& Event)
{
	Scroll.CaptureLost();
	SImGuiContextWidget::OnMouseCaptureLost(Event);
}
FReply SImGuiAssetView::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
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
		PendingActions.Add([Weak = TWeakPtr<SImGuiAssetView>(SharedThis(this)), Item]()
		                   {
			if (const auto View = Weak.Pin())
			{
				if (!Item || !View->Selected.Contains(Item))
				{
					View->ClearSelection();
					if (Item)
					{
						View->SetSelection(Item, true, ESelectInfo::OnMouseClick);
					}
				}
				View->ShowContextMenu();
			} });
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Reply;
}

FReply SImGuiAssetView::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	FReply Reply = SImGuiContextWidget::OnMouseButtonDoubleClick(Geometry, Event);
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && !Rename.IsActive())
	{
		if (const auto Item = ItemAt(Event.GetScreenSpacePosition()))
		{
			PendingActions.Add([Weak = TWeakPtr<SImGuiAssetView>(SharedThis(this)), Item]()
			                   {
				if (const auto View = Weak.Pin())
				{
					View->ClearSelection();
					View->SetSelection(Item, true, ESelectInfo::OnMouseClick);
					{
						FTBImGuiContextScope Scope(View->OwningContext);
						if (Scope.IsValid())
						{
							ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
						}
					}
					const auto Backend = View->FindOwner();
					if (Backend && Backend->IsThumbnailEditMode())
					{
						View->ShowThumbnailEditor(Item);
					}
					else
					{
						View->DoubleClicked.ExecuteIfBound(Item);
					}
				} });
		}
	}
	return Reply;
}

FReply SImGuiAssetView::OnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const auto Backend = FindOwner();
	const auto Item = DragSourceItem.Pin();
	if (!Backend || !Item)
	{
		return FReply::Unhandled();
	}
	if (!Selected.Contains(Item))
	{
		Selected.Reset();
		SelectedPaths.Reset();
		AssignSelected(Item, true);
		SelectionChanged.ExecuteIfBound(Item, ESelectInfo::OnMouseClick);
	}
	FReply Reply = Backend->HandleCustomViewDrag(Geometry, Event);
	if (Reply.IsEventHandled())
	{
		FTBImGuiContextScope Scope(OwningContext);
		if (Scope.IsValid())
		{
			ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
		}
	}
	return Reply;
}

FReply SImGuiAssetView::OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	const auto Backend = FindOwner();
	const auto Item = ItemAt(Event.GetScreenSpacePosition());
	if (const auto Previous = DragHoverItem.Pin(); Backend && Previous && Previous != Item)
	{
		Backend->HandleCustomViewDragLeave(Previous->GetItem(), Event);
		DragHoverItem.Reset();
	}
	if (Backend && Item && Backend->HandleCustomViewDragOver(Item->GetItem(), Event))
	{
		if (DragHoverItem.Pin() != Item)
		{
			DragHoverStart = FPlatformTime::Seconds();
		}
		DragHoverItem = Item;
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SImGuiAssetView::OnDragLeave(const FDragDropEvent& Event)
{
	const auto Backend = FindOwner();
	if (const auto Item = DragHoverItem.Pin(); Backend && Item)
	{
		Backend->HandleCustomViewDragLeave(Item->GetItem(), Event);
	}
	DragHoverItem.Reset();
	SImGuiContextWidget::OnDragLeave(Event);
}

FReply SImGuiAssetView::OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	const auto Backend = FindOwner();
	const auto Item = ItemAt(Event.GetScreenSpacePosition());
	const bool bHandled = Backend && Item && Backend->HandleCustomViewDrop(Item->GetItem(), Event);
	DragHoverItem.Reset();
	return bHandled ? FReply::Handled() : FReply::Unhandled();
}

FReply SImGuiAssetView::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	FTBImGuiContextScope Scope(OwningContext);
	FReply ImGuiReply = SImGuiContextWidget::OnKeyDown(Geometry, Event);
	if (Scope.IsValid() && !ImGui::GetIO().WantTextInput)
	{
		const FKey Key = Event.GetKey();
		const bool bNavigation = Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Home || Key == EKeys::End || Key == EKeys::PageUp || Key == EKeys::PageDown || ((Event.IsControlDown() || Event.IsCommandDown()) && Key == EKeys::A);
		if (Event.IsShiftDown() && Key == EKeys::F10)
		{
			ShowContextMenu();
			return FReply::Handled();
		}
		if ((Event.IsControlDown() || Event.IsCommandDown()) && Key == EKeys::SpaceBar)
		{
			SelectIndex(Items.IndexOfByKey(NavigationItem.Pin()), true, false, ESelectInfo::OnKeyPress);
			return FReply::Handled();
		}
		if (!bNavigation)
		{
			if (const auto Backend = FindOwner())
			{
				FReply Reply = Backend->OnKeyDown(Geometry, Event);
				if (Reply.IsEventHandled())
				{
					return Reply;
				}
			}
		}
		return bNavigation ? FReply::Handled() : FReply::Unhandled();
	}
	return ImGuiReply;
}

FReply SImGuiAssetView::OnKeyChar(const FGeometry& Geometry, const FCharacterEvent& Event)
{
	FTBImGuiContextScope Scope(OwningContext);
	if (Scope.IsValid() && !ImGui::GetIO().WantTextInput)
	{
		if (const auto Backend = FindOwner())
		{
			return Backend->OnKeyChar(Geometry, Event);
		}
	}
	return SImGuiContextWidget::OnKeyChar(Geometry, Event);
}

FReply SImGuiAssetView::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.IsControlDown() && bGrid)
	{
		TileSize = FMath::Clamp(TileSize + Event.GetWheelDelta() * 8.0f, 32.0f, 256.0f);
		bGridLabelsDirty = true;
		GetMutableDefault<UTBImGuiEditorSettings>()->ContentBrowserThumbnailSize = TileSize;
		bSaveThumbnailSize = true;
		return FReply::Handled();
	}
	return SImGuiContextWidget::OnMouseWheel(Geometry, Event);
}
