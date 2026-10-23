// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiDetailsView.h"

#include "DetailTreeNode.h"
#include "DetailAssetView.h"
#include "TBImGuiAssetThumbnails.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IMenu.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "InputCoreTypes.h"
#include "Layout/WidgetPath.h"
#include "TBImGuiEditor.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"

namespace TBIMGUI_DETAILS_VIEW
{
static bool IsResetVisible(const TSharedRef<IPropertyHandle>& Handle, const FDetailWidgetRow& Metadata)
{
	if (Metadata.CustomResetToDefault.IsSet())
	{
		return Metadata.CustomResetToDefault->IsResetToDefaultVisible(Handle);
	}
	return !Handle->HasMetaData(TEXT("NoResetToDefault")) && !Handle->GetInstanceMetaData(TEXT("NoResetToDefault")) && Handle->CanResetToDefault();
}

static bool ContainsNode(const TArray<TSharedRef<FDetailTreeNode>>& Nodes, const FDetailTreeNode* Target)
{
	for (const TSharedRef<FDetailTreeNode>& Node : Nodes)
	{
		if (&Node.Get() == Target)
		{
			return true;
		}
		FDetailNodeList Children;
		Node->GetChildren(Children);
		if (ContainsNode(Children, Target))
		{
			return true;
		}
	}
	return false;
}
} // namespace TBIMGUI_DETAILS_VIEW

void SImGuiDetailsView::Construct(const FArguments& Args)
{
	AssetThumbnails = MakeUnique<FTBImGuiAssetThumbnails>();
	check(Args._DetailsView.IsValid());
	WeakDetailsView = Args._DetailsView;
	TreeContext = Args._TreeContext;
	FTBImGuiContextParams Params(FString::Printf(TEXT("Details%08x"), GetTypeHash(Args._DetailsView.Get())));
	Params.ViewportMode = ETBImGuiViewportMode::Single;
	Params.bDockingSupport = false;
	SImGuiContextWidget::Construct(SImGuiContextWidget::FArguments().ContextParams(Params));
	SetCanTick(true);
}

SImGuiDetailsView::~SImGuiDetailsView()
{
	if (NativeAssetPicker)
	{
		NativeAssetPicker->ClosePicker.ExecuteIfBound();
	}
	if (const TSharedPtr<IMenu> Menu = NativeMenu.Pin())
	{
		Menu->Dismiss();
	}
	if (OwningContext)
	{
		OwningContext->DestroyContext();
	}
}

bool SImGuiDetailsView::IsCurrentNode(const TSharedRef<FDetailTreeNode>& Node) const
{
	return WeakDetailsView.IsValid() && TBIMGUI_DETAILS_VIEW::ContainsNode(TreeContext.RootNodes.Get(), &Node.Get());
}

bool SImGuiDetailsView::CanEdit(const TSharedRef<FDetailTreeNode>& Node, const FRowState& Row, bool bCheckCondition) const
{
	const TSharedPtr<IDetailsView> View = WeakDetailsView.Pin();
	return View && View->IsPropertyEditingEnabled() && Node->IsPropertyEditingEnabled().Get(false) && Row.Handle && Row.Handle->IsValidHandle() && !Node->IsCustomViewReadOnly() && Row.Metadata.IsEnabledAttr.Get(true) && Row.Metadata.IsValueEnabledAttr.Get(true) && (!bCheckCondition || Row.Metadata.EditConditionValue.Get(true));
}

void SImGuiDetailsView::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
	if (NativeAssetPicker && !NativeMenu.IsValid())
	{
		NativeAssetPicker->ClosePicker.ExecuteIfBound();
		NativeAssetPicker.Reset();
	}
	AssetThumbnails->Tick();
	AssetHits.Reset();
	if (const TSharedPtr<IMenu> Menu = NativeMenu.Pin())
	{
		const TSharedPtr<FDetailTreeNode> Node = NativeMenuNode.Pin();
		if (!Node || !IsCurrentNode(Node.ToSharedRef()))
		{
			Menu->Dismiss();
			NativeMenu.Reset();
		}
	}
	SetContentSize(Geometry.GetLocalSize());
	++FrameNumber;
	SImGuiContextWidget::Tick(Geometry, CurrentTime, DeltaTime);
	TArray<TFunction<void()>> Actions = MoveTemp(PendingActions);
	PendingActions.Reset();
	for (TFunction<void()>& Action : Actions)
	{
		Action();
	}
	for (auto It = RowStates.CreateIterator(); It; ++It)
	{
		if (!It.Value()->Node.IsValid() || It.Value()->LastDrawn != FrameNumber)
		{
			It.RemoveCurrent();
		}
	}
}

FReply SImGuiDetailsView::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	FTBImGuiContextScope Scope(OwningContext);
	const FReply Reply = SImGuiContextWidget::OnKeyDown(Geometry, Event);
	if (Scope.IsValid() && (ImGui::GetIO().WantTextInput || Event.GetKey() == EKeys::Tab || Event.GetKey() == EKeys::Up || Event.GetKey() == EKeys::Down || Event.GetKey() == EKeys::Left || Event.GetKey() == EKeys::Right || Event.GetKey() == EKeys::Enter || Event.GetKey() == EKeys::SpaceBar || Event.GetKey() == EKeys::Escape))
	{
		return Reply;
	}
	return FReply::Unhandled();
}

void SImGuiDetailsView::Queue(TSharedRef<FDetailTreeNode> Node, TFunction<void(TSharedRef<FDetailTreeNode>, TSharedRef<IDetailsView>)> Action)
{
	auto Execute = [this, WeakNode = TWeakPtr<FDetailTreeNode>(Node), Action = MoveTemp(Action)]()
	{
		const TSharedPtr<FDetailTreeNode> CurrentNode = WeakNode.Pin();
		const TSharedPtr<IDetailsView> View = WeakDetailsView.Pin();
		if (CurrentNode && View && IsCurrentNode(CurrentNode.ToSharedRef()))
		{
			Action(CurrentNode.ToSharedRef(), View.ToSharedRef());
		}
	};
	PendingActions.Add(MoveTemp(Execute));
}

TSharedRef<SImGuiDetailsView::FRowState> SImGuiDetailsView::FindRow(TSharedRef<FDetailTreeNode> Node)
{
	TSharedPtr<FRowState>& Row = RowStates.FindOrAdd(&Node.Get());
	if (!Row || Row->Node.Pin() != Node)
	{
		Row = MakeShared<FRowState>();
		Row->Node = Node;
		Row->Handle = Node->CreatePropertyHandle();
		Row->bDefaultValue = Node->GetCustomViewRow(Row->Metadata) && FTBImGuiPropertyEditor::Supports(Row->Handle);
	}
	Row->LastDrawn = FrameNumber;
	return Row.ToSharedRef();
}

void SImGuiDetailsView::ShowNativeEditor(TSharedRef<FDetailTreeNode> Node)
{
	DismissAssetPicker();
	if (!TreeContext.CreateNativeRow.IsBound())
	{
		return;
	}
	if (const TSharedPtr<IMenu> Menu = NativeMenu.Pin())
	{
		Menu->Dismiss();
	}
	const TSharedRef<SWidget> Content = TreeContext.CreateNativeRow.Execute(Node);
	// clang-format off
	const TSharedRef<SWidget> Popup = SNew(SBorder)
		.Padding(8.0f)
		[
			SNew(SBox)
			.WidthOverride(FMath::Max(480.0f, GetCachedGeometry().GetLocalSize().X))
			[
				Content
			]
		];
	// clang-format on
	NativeMenuNode = Node;
	NativeMenu = FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Popup,
	                                               FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
}

void SImGuiDetailsView::QueueValue(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row, const FString& Value)
{
	auto ApplyValue = [this, Row, Value](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView> View)
	{
		if (CanEdit(CurrentNode, *Row))
		{
			Row->Editor.Error = FTBImGuiPropertyEditor::Apply(Row->Handle.ToSharedRef(), Value)
			                        ? FString()
			                        : TEXT("The value was rejected. Check its type, allowed values, and minimum/maximum limits.");
		}
	};
	Queue(Node, MoveTemp(ApplyValue));
}

void SImGuiDetailsView::ResetValue(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row)
{
	auto Reset = [this, Row](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView> View)
	{
		if (!CanEdit(CurrentNode, *Row) || !TBIMGUI_DETAILS_VIEW::IsResetVisible(Row->Handle.ToSharedRef(), Row->Metadata))
		{
			return;
		}
		const TOptional<FResetToDefaultOverride>& Override = Row->Metadata.CustomResetToDefault;
		if (Override.IsSet() && Override->HasResetToDefaultHandler())
		{
			Override->OnResetToDefaultClicked(Row->Handle);
		}
		else
		{
			Row->Handle->ResetToDefault();
		}
		Row->Editor.Error.Reset();
	};
	Queue(Node, MoveTemp(Reset));
}

void SImGuiDetailsView::QueueNativeEditor(TSharedRef<FDetailTreeNode> Node)
{
	auto OpenEditor = [this](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView> View)
	{
		ShowNativeEditor(CurrentNode);
	};
	Queue(Node, MoveTemp(OpenEditor));
}

void SImGuiDetailsView::DrawActions(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row)
{
	if (ImGui::BeginPopup("Actions"))
	{
		FString Value;
		const bool bHasValue = Row->Handle && Row->Handle->IsValidHandle() && Row->Handle->GetValueAsFormattedString(Value) == FPropertyAccess::Success;
		if (bHasValue && Row->Handle->GetProperty()->IsA<FStrProperty>())
		{
			Row->Handle->GetValue(Value);
		}
		else if (bHasValue && Row->Handle->GetProperty()->IsA<FNameProperty>())
		{
			FName Name;
			Row->Handle->GetValue(Name);
			Value = Name.ToString();
		}
		if (ImGui::MenuItem("Copy value", nullptr, false, bHasValue))
		{
			FPlatformApplicationMisc::ClipboardCopy(*Value);
		}
		if (ImGui::MenuItem("Paste value", nullptr, false, Row->bDefaultValue && CanEdit(Node, *Row)))
		{
			FString Pasted;
			FPlatformApplicationMisc::ClipboardPaste(Pasted);
			QueueValue(Node, Row, Pasted);
		}
		if (ImGui::MenuItem("Native property editor..."))
		{
			QueueNativeEditor(Node);
		}
		ImGui::EndPopup();
	}
}

void SImGuiDetailsView::DrawNode(TSharedRef<FDetailTreeNode> Node)
{
	if (Node->GetVisibility() != ENodeVisibility::Visible)
	{
		return;
	}
	FDetailNodeList Children;
	Node->GetChildren(Children);
	if (Node->ShouldShowOnlyChildren())
	{
		for (const TSharedRef<FDetailTreeNode>& Child : Children)
		{
			DrawNode(Child);
		}
		return;
	}
	ImGui::PushID(&Node.Get());
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	const bool bCategory = Node->GetNodeType() == EDetailNodeType::Category;
	const bool bAdvanced = Node->GetNodeType() == EDetailNodeType::Advanced;
	const bool bBranch = !Children.IsEmpty();
	const bool bExpanded = TreeContext.IsExpanded.IsBound() && TreeContext.IsExpanded.Execute(Node);
	ImGui::SetNextItemOpen(bExpanded, ImGuiCond_Always);
	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;
	if (!bBranch && !bAdvanced)
	{
		Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	const FString Label = bAdvanced ? TEXT("Advanced") : Node->GetCustomViewDisplayName().ToString();
	// AK CHANGES - Category headers
	const bool bOpen = bCategory
	                       ? ImGui::CollapsingHeader(TCHAR_TO_UTF8(*Label), ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns)
	                       : ImGui::TreeNodeEx("##Name", Flags, "%s", TCHAR_TO_UTF8(*Label));
	if (ImGui::IsItemToggledOpen() && TreeContext.SetExpanded.IsBound())
	{
		const bool bRecursive = ImGui::GetIO().KeyShift && !bAdvanced;
		auto Expand = [this, bOpen, bRecursive](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView> View)
		{
			TreeContext.SetExpanded.ExecuteIfBound(CurrentNode, bOpen, bRecursive);
		};
		Queue(Node, MoveTemp(Expand));
	}
	const TSharedRef<FRowState> Row = FindRow(Node);
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("Actions");
	}
	if (Row->Handle && Row->Handle->IsValidHandle() && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", TCHAR_TO_UTF8(*Row->Handle->GetToolTipText().ToString()));
	}
	DrawActions(Node, Row);
	if (bCategory)
	{
		if (bOpen)
		{
			for (const TSharedRef<FDetailTreeNode>& Child : Children)
			{
				DrawNode(Child);
			}
		}
		ImGui::PopID();
		return;
	}
	const bool bValueVisible = ImGui::TableSetColumnIndex(1);
	if (bValueVisible && !Row->Metadata.CustomViewActions.IsEmpty())
	{
		const TSharedPtr<IDetailsView> View = WeakDetailsView.Pin();
		const bool bEnabled = View && View->IsPropertyEditingEnabled() && Node->IsPropertyEditingEnabled().Get(false) && Row->Metadata.IsEnabledAttr.Get(true) && Row->Metadata.IsValueEnabledAttr.Get(true) && Row->Metadata.EditConditionValue.Get(true);
		for (int32 ActionIndex = 0; ActionIndex < Row->Metadata.CustomViewActions.Num(); ++ActionIndex)
		{
			const FDetailWidgetRow::FCustomViewAction& Entry = Row->Metadata.CustomViewActions[ActionIndex];
			ImGui::PushID(ActionIndex);
			ImGui::BeginDisabled(!bEnabled || !Entry.Action.CanExecute());
			if (ImGui::Button(TCHAR_TO_UTF8(*Entry.Label.ToString())))
			{
				auto ExecuteFunction = [Row, Action = Entry.Action](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView> CurrentView)
				{
					if (CurrentView->IsPropertyEditingEnabled() && CurrentNode->IsPropertyEditingEnabled().Get(false) && Row->Metadata.IsEnabledAttr.Get(true) && Row->Metadata.IsValueEnabledAttr.Get(true) && Row->Metadata.EditConditionValue.Get(true))
					{
						Action.Execute();
					}
				};
				Queue(Node, MoveTemp(ExecuteFunction));
			}
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("%s", TCHAR_TO_UTF8(*Entry.ToolTip.ToString()));
			}
			ImGui::PopID();
		}
	}
	else if (bValueVisible && Row->Metadata.CustomViewAsset)
	{
		DrawAsset(Node, Row);
	}
	else if ((bValueVisible || Row->Editor.IsEditing()) && Row->bDefaultValue && Row->Handle->IsValidHandle())
	{
		if (Row->Metadata.OnEditConditionValueChanged.IsBound())
		{
			bool bCondition = Row->Metadata.EditConditionValue.Get(true);
			ImGui::BeginDisabled(!CanEdit(Node, *Row, false));
			if (ImGui::Checkbox("##Condition", &bCondition))
			{
				auto SetCondition = [this, Row, bCondition](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView> View)
				{
					if (CanEdit(CurrentNode, *Row, false))
					{
						Row->Metadata.OnEditConditionValueChanged.ExecuteIfBound(bCondition);
					}
				};
				Queue(Node, MoveTemp(SetCondition));
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
		}
		ImGui::BeginDisabled(!CanEdit(Node, *Row));
		ImGui::SetNextItemWidth(-1.0f);
		if (const TOptional<FString> Value = Row->Editor.Draw(Row->Handle.ToSharedRef()); Value.IsSet())
		{
			QueueValue(Node, Row, Value.GetValue());
		}
		ImGui::EndDisabled();
	}
	else if (bValueVisible && !bAdvanced && Node->GetNodeType() == EDetailNodeType::Item)
	{
		FString Value;
		if (Row->Handle && Row->Handle->IsValidHandle())
		{
			Row->Handle->GetValueAsDisplayString(Value);
		}
		if (ImGui::Button("Edit..."))
		{
			QueueNativeEditor(Node);
		}
		if (!Value.IsEmpty())
		{
			ImGui::SameLine();
			ImGui::TextUnformatted(TCHAR_TO_UTF8(*Value));
		}
	}
	const bool bActionsVisible = ImGui::TableSetColumnIndex(2);
	if (bActionsVisible && Row->bDefaultValue && Row->Handle->IsValidHandle())
	{
		const bool bShowReset = TBIMGUI_DETAILS_VIEW::IsResetVisible(Row->Handle.ToSharedRef(), Row->Metadata);
		ImGui::BeginDisabled(!CanEdit(Node, *Row));
		if (bShowReset && ImGui::SmallButton("Reset"))
		{
			ResetValue(Node, Row);
		}
		ImGui::EndDisabled();
	}
	if (bActionsVisible && !Row->Editor.Error.IsEmpty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "!");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", TCHAR_TO_UTF8(*Row->Editor.Error));
		}
	}
	if (bOpen && (bBranch || bAdvanced))
	{
		for (const TSharedRef<FDetailTreeNode>& Child : Children)
		{
			DrawNode(Child);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void SImGuiDetailsView::DrawImGuiContent()
{
	if (!bThemeApplied)
	{
		FTBImGuiEditorModule::ApplyThemeToCurrentContext();
		bThemeApplied = true;
	}
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->Pos);
	ImGui::SetNextWindowSize(Viewport->Size);
	constexpr ImGuiWindowFlags FLAGS = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin("##Details", nullptr, FLAGS))
	{
		if (ImGui::BeginTable("Properties", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.45f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.55f);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 50.0f);
			for (const TSharedRef<FDetailTreeNode>& Node : TreeContext.RootNodes.Get())
			{
				DrawNode(Node);
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}
