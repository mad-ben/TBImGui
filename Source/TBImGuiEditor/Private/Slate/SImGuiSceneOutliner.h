// Copyright Lewis Scrivens. All Rights Reserved.
#pragma once

#include "SceneOutlinerFwd.h"
#include "Slate/SImGuiContextWidget.h"
#include "Slate/TBImGuiInlineRename.h"
#include "Slate/TBImGuiEditorScroll.h"

class SSceneOutliner;
class FScopedTransaction;

/** ImGui presentation of Unreal's existing World Outliner model. */
class SImGuiSceneOutliner final : public SImGuiContextWidget
{
public:
	SLATE_BEGIN_ARGS(SImGuiSceneOutliner) {}
	SLATE_ARGUMENT(TSharedPtr<SSceneOutliner>, Outliner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override;
	virtual FReply OnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event) override;
	virtual void OnDragLeave(const FDragDropEvent& Event) override;
	virtual FReply OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event) override;
	virtual TSharedPtr<IToolTip> GetToolTip() override;

protected:
	virtual void DrawImGuiContent() override;

private:
	struct FRow
	{
		FSceneOutlinerTreeItemPtr Item;
		int32 Depth;
		bool bHasChildren;
	};
	struct FHitItem
	{
		FSceneOutlinerTreeItemPtr Item;
		FSlateRect Rect;
		FSlateRect NameRect;
	};
	void AppendRows(const TArrayView<const FSceneOutlinerTreeItemPtr>& Items, int32 Depth);
	void SelectRow(int32 Index, bool bToggle, bool bRange, ESelectInfo::Type SelectInfo = ESelectInfo::OnMouseClick);
	void DrawRow(int32 Index);
	FSceneOutlinerTreeItemPtr ItemAt(FVector2D Position) const;
	FReply HandleDrop(const FDragDropEvent& Event, bool bApplyDrop);
	void ShowContextMenu();
	void ShowRenameEditor(TWeakPtr<ISceneOutlinerTreeItem> WeakItem);
	void HandleNavigation();
	void ShowColumnEditor(FName ColumnId, FSceneOutlinerTreeItemPtr Item);

	TWeakPtr<SSceneOutliner> WeakOutliner;
	TWeakPtr<ISceneOutlinerTreeItem> SelectionAnchor;
	TWeakPtr<ISceneOutlinerTreeItem> NavigationItem;
	TArray<FRow> Rows;
	TArray<FHitItem> HitItems;
	TArray<FName> ColumnIds;
	TWeakPtr<ISceneOutlinerTreeItem> DragSourceItem;
	TWeakPtr<ISceneOutlinerTreeItem> DragHoverItem;
	TWeakPtr<ISceneOutlinerTreeItem> ActiveRename;
	FTBImGuiInlineRename Rename;
	FTBImGuiEditorScroll Scroll;
	TSharedPtr<FScopedTransaction> VisibilityTransaction;
	TSet<const ISceneOutlinerTreeItem*> VisibilityPaintedItems;
	TWeakPtr<ISceneOutlinerTreeItem> ToolTipItem;
	TSharedPtr<SWidget> ToolTipSource;
	double DragHoverStart = 0.0;
	TArray<TFunction<void()>> PendingActions;
	TSet<const ISceneOutlinerTreeItem*> SelectedItems;
	bool bThemeApplied = false;
	bool bRenameDrawn = false;
	bool bVisibilityPainting = false;
	bool bVisibilityPaintValue = false;
	bool bEndVisibilityPainting = false;
};
