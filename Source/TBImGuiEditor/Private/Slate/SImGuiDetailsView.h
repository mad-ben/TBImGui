// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include "DetailsViewTreeContent.h"
#include "DetailWidgetRow.h"
#include "Slate/SImGuiContextWidget.h"
#include "Slate/TBImGuiPropertyEditor.h"

class IMenu;
class FTBImGuiAssetThumbnails;

class SImGuiDetailsView final : public SImGuiContextWidget
{
public:
	SLATE_BEGIN_ARGS(SImGuiDetailsView) {}
	SLATE_ARGUMENT(TSharedPtr<IDetailsView>, DetailsView)
	SLATE_ARGUMENT(FDetailsViewTreeContext, TreeContext)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	virtual ~SImGuiDetailsView() override;
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event) override;
	virtual FReply OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event) override;

protected:
	virtual void DrawImGuiContent() override;

private:
	struct FRowState
	{
		TWeakPtr<FDetailTreeNode> Node;
		TSharedPtr<IPropertyHandle> Handle;
		FDetailWidgetRow Metadata;
		FTBImGuiPropertyEditor Editor;
		bool bDefaultValue = false;
		uint64 LastDrawn = 0;
	};

	void DrawNode(TSharedRef<FDetailTreeNode> Node);
	TSharedRef<FRowState> FindRow(TSharedRef<FDetailTreeNode> Node);
	void Queue(TSharedRef<FDetailTreeNode> Node, TFunction<void(TSharedRef<FDetailTreeNode>, TSharedRef<IDetailsView>)> Action);
	void ShowNativeEditor(TSharedRef<FDetailTreeNode> Node);
	void QueueNativeEditor(TSharedRef<FDetailTreeNode> Node);
	bool IsCurrentNode(const TSharedRef<FDetailTreeNode>& Node) const;
	bool CanEdit(const TSharedRef<FDetailTreeNode>& Node, const FRowState& Row, bool bCheckCondition = true) const;
	void QueueValue(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row, const FString& Value);
	void DrawActions(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row);
	void ResetValue(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row);
	bool CanEditAsset(TSharedRef<FDetailTreeNode> Node, const FRowState& Row) const;
	void DrawAsset(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row);
	void ShowAssetPicker(TSharedRef<FDetailTreeNode> Node, TSharedRef<FRowState> Row);
	void DismissAssetPicker();
	FReply HandleAssetDrop(const FDragDropEvent& Event, bool bApply);

	struct FAssetHit
	{
		FSlateRect Rect;
		TWeakPtr<FDetailTreeNode> Node;
	};
	TArray<FAssetHit> AssetHits;
	TUniquePtr<FTBImGuiAssetThumbnails> AssetThumbnails;
	TSharedPtr<FDetailAssetView> NativeAssetPicker;

	TWeakPtr<IDetailsView> WeakDetailsView;
	FDetailsViewTreeContext TreeContext;
	TMap<const FDetailTreeNode*, TSharedPtr<FRowState>> RowStates;
	TArray<TFunction<void()>> PendingActions;
	TWeakPtr<IMenu> NativeMenu;
	TWeakPtr<FDetailTreeNode> NativeMenuNode;
	uint64 FrameNumber = 0;
	bool bThemeApplied = false;
};
