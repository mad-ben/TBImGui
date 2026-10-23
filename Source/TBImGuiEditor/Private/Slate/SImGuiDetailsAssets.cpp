// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiDetailsView.h"

#include "DetailAssetView.h"
#include "DetailTreeNode.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IMenu.h"
#include "IDetailsView.h"
#include "Layout/WidgetPath.h"
#include "TBImGuiAssetThumbnails.h"

bool SImGuiDetailsView::CanEditAsset(TSharedRef<FDetailTreeNode> Node, const FRowState& Row) const
{
	const TSharedPtr<IDetailsView> View = WeakDetailsView.Pin();
	return View && View->IsPropertyEditingEnabled() && Node->IsPropertyEditingEnabled().Get(false) && Row.Metadata.IsEnabledAttr.Get(true) && Row.Metadata.IsValueEnabledAttr.Get(true) && Row.Metadata.EditConditionValue.Get(true) && Row.Metadata.CustomViewAsset && Row.Metadata.CustomViewAsset->CanEdit();
}

void SImGuiDetailsView::DismissAssetPicker()
{
	if (const TSharedPtr<FDetailAssetView> Picker = MoveTemp(NativeAssetPicker))
	{
		Picker->ClosePicker.ExecuteIfBound();
		if (const TSharedPtr<IMenu> Menu = NativeMenu.Pin())
		{
			Menu->Dismiss();
		}
		NativeMenu.Reset();
	}
}

void SImGuiDetailsView::ShowAssetPicker(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row)
{
	if (!CanEditAsset(Node, *Row))
	{
		return;
	}
	DismissAssetPicker();
	if (const TSharedPtr<IMenu> Menu = NativeMenu.Pin())
	{
		Menu->Dismiss();
	}
	FWidgetPath OwnerPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(AsShared(), OwnerPath);
	if (!OwnerPath.IsValid())
	{
		return;
	}
	NativeAssetPicker = Row->Metadata.CustomViewAsset;
	const auto IsCurrent = [Weak = TWeakPtr<SImGuiDetailsView>(SharedThis(this)), WeakNode = TWeakPtr<FDetailTreeNode>(Node), WeakRow = TWeakPtr<FRowState>(Row)]()
	{
		const TSharedPtr<SImGuiDetailsView> View = Weak.Pin();
		const TSharedPtr<FDetailTreeNode> CurrentNode = WeakNode.Pin();
		const TSharedPtr<FRowState> CurrentRow = WeakRow.Pin();
		return View && CurrentNode && CurrentRow && View->IsCurrentNode(CurrentNode.ToSharedRef()) && View->CanEditAsset(CurrentNode.ToSharedRef(), *CurrentRow);
	};
	const TSharedRef<SWidget> Picker = NativeAssetPicker->MakePicker(FSimpleDelegate::CreateSP(this, &SImGuiDetailsView::DismissAssetPicker), IsCurrent);
	NativeMenuNode = Node;
	NativeMenu = FSlateApplication::Get().PushMenu(AsShared(), OwnerPath, Picker, FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
}

void SImGuiDetailsView::DrawAsset(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row)
{
	const TSharedRef<FDetailAssetView> Model = Row->Metadata.CustomViewAsset.ToSharedRef();
	const FAssetData Asset = Model->GetAsset();
	const bool bEditable = CanEditAsset(Node, *Row);
	const ImVec2 Start = ImGui::GetCursorScreenPos();
	const float Width = ImGui::GetContentRegionAvail().x;
	ImGui::BeginGroup();
	if (UTexture* Texture = AssetThumbnails->FindOrQueue(Asset))
	{
		ImGui::ImageButton("##AssetPreview", Texture, ImVec2(40.0f, 40.0f));
	}
	else
	{
		ImGui::Button("##AssetPreview", ImVec2(48.0f, 48.0f));
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", TCHAR_TO_UTF8(*Asset.GetObjectPathString()));
		if (Asset.IsValid() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			Queue(Node, [Model](TSharedRef<FDetailTreeNode>, TSharedRef<IDetailsView>)
			      { Model->Open.ExecuteIfBound(); });
		}
	}
	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginDisabled(!bEditable);
	const FString Label = Model->GetName().ToString() + TEXT("  v");
	if (ImGui::Button(TCHAR_TO_UTF8(*Label), ImVec2(FMath::Max(48.0f, Width - 60.0f), 0.0f)))
	{
		Queue(Node, [this, Row](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView>)
		      { ShowAssetPicker(CurrentNode, Row); });
	}
	if (Model->bDisplayUseSelected && ImGui::SmallButton("Use"))
	{
		Queue(Node, [this, Row, Model](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView>)
		      {
			if (CanEditAsset(CurrentNode, *Row))
			{
				Model->UseSelected.ExecuteIfBound();
			} });
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!Asset.IsValid());
	if (ImGui::SmallButton("Browse"))
	{
		Queue(Node, [Model](TSharedRef<FDetailTreeNode>, TSharedRef<IDetailsView>)
		      { Model->Browse.ExecuteIfBound(); });
	}
	ImGui::EndDisabled();
	ImGui::BeginDisabled(!bEditable);
	if (Model->bAllowClear)
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear"))
		{
			Queue(Node, [this, Row, Model](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView>)
			      {
				if (CanEditAsset(CurrentNode, *Row))
				{
					Model->Clear.ExecuteIfBound();
				} });
		}
	}
	const bool bHasHandle = Row->Handle && Row->Handle->IsValidHandle();
	const TOptional<FResetToDefaultOverride>& Override = Row->Metadata.CustomResetToDefault;
	const bool bShowReset = Override.IsSet() ? Override->IsResetToDefaultVisible(Row->Handle) : bHasHandle && !Row->Handle->HasMetaData(TEXT("NoResetToDefault")) && !Row->Handle->GetInstanceMetaData(TEXT("NoResetToDefault")) && Row->Handle->CanResetToDefault();
	if (bShowReset)
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("Reset"))
		{
			Queue(Node, [this, Row](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView>)
			      {
				if (CanEditAsset(CurrentNode, *Row))
				{
					const TOptional<FResetToDefaultOverride>& Reset = Row->Metadata.CustomResetToDefault;
					if (Reset.IsSet() && Reset->HasResetToDefaultHandler())
					{
						Reset->OnResetToDefaultClicked(Row->Handle);
					}
					else if (Row->Handle && Row->Handle->IsValidHandle())
					{
						Row->Handle->ResetToDefault();
					}
				} });
		}
	}
	ImGui::EndDisabled();
	ImGui::EndGroup();
	ImGui::EndGroup();
	const ImVec2 End = ImGui::GetItemRectMax();
	const ImVec2 ClipMin = ImGui::GetWindowDrawList()->GetClipRectMin();
	const ImVec2 ClipMax = ImGui::GetWindowDrawList()->GetClipRectMax();
	if (End.y > ClipMin.y && Start.y < ClipMax.y)
	{
		const FVector2D Min = GetCachedGeometry().LocalToAbsolute(FVector2D(FMath::Max(Start.x, ClipMin.x), FMath::Max(Start.y, ClipMin.y)));
		const FVector2D Max = GetCachedGeometry().LocalToAbsolute(FVector2D(FMath::Min(Start.x + Width, ClipMax.x), FMath::Min(End.y, ClipMax.y)));
		AssetHits.Add({FSlateRect(Min.X, Min.Y, Max.X, Max.Y), Node});
	}
}

FReply SImGuiDetailsView::HandleAssetDrop(const FDragDropEvent& Event, bool bApply)
{
	const TSharedPtr<FAssetDragDropOp> Operation = Event.GetOperationAs<FAssetDragDropOp>();
	if (!Operation || Operation->GetAssets().Num() != 1)
	{
		return FReply::Unhandled();
	}
	for (const FAssetHit& Hit : AssetHits)
	{
		const TSharedPtr<FDetailTreeNode> Node = Hit.Node.Pin();
		if (!Hit.Rect.ContainsPoint(Event.GetScreenSpacePosition()) || !Node || !IsCurrentNode(Node.ToSharedRef()))
		{
			continue;
		}
		const TSharedRef<FRowState> Row = FindRow(Node.ToSharedRef());
		const TSharedPtr<FDetailAssetView> Model = Row->Metadata.CustomViewAsset;
		FText Reason;
		const FAssetData Asset = Operation->GetAssets()[0];
		if (!Model || !CanEditAsset(Node.ToSharedRef(), *Row) || !Model->CanAssign(Asset, Reason))
		{
			return FReply::Unhandled();
		}
		if (bApply)
		{
			Queue(Node.ToSharedRef(), [this, Row, Model, Asset](TSharedRef<FDetailTreeNode> CurrentNode, TSharedRef<IDetailsView>)
			      {
				FText Failure;
				if (CanEditAsset(CurrentNode, *Row) && Model->CanAssign(Asset, Failure))
				{
					Model->Assign(Asset);
				} });
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SImGuiDetailsView::OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	return HandleAssetDrop(Event, false);
}

FReply SImGuiDetailsView::OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event)
{
	return HandleAssetDrop(Event, true);
}
