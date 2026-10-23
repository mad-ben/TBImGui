// Copyright Lewis Scrivens. All Rights Reserved.
#pragma once

#include "Experimental/ContentBrowserViewExtender.h"
#include "Slate/SImGuiContextWidget.h"
#include "Slate/TBImGuiInlineRename.h"
#include "Slate/TBImGuiEditorScroll.h"

class FTBImGuiAssetThumbnails;
class SAssetView;

/** A Content Browser view consuming Unreal's filtered and sorted items. */
class SImGuiAssetView final : public SImGuiContextWidget, public IContentBrowserViewExtender
{
public:
	SLATE_BEGIN_ARGS(SImGuiAssetView) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual ~SImGuiAssetView() override;
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply OnKeyChar(const FGeometry& Geometry, const FCharacterEvent& Event) override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
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
	virtual bool OnVisualizeTooltip(const TSharedPtr<SWidget>& Content) override;
	virtual void OnToolTipClosing() override;

	virtual TSharedRef<SWidget> CreateView(TArray<TSharedPtr<FAssetViewItem>>* Items) override;
	virtual void OnItemListChanged(TArray<TSharedPtr<FAssetViewItem>>* Items) override;
	virtual FOnSelectionChanged& OnSelectionChanged() override
	{
		return SelectionChanged;
	}
	virtual FOnItemScrolledIntoView& OnItemScrolledIntoView() override
	{
		return ScrolledIntoView;
	}
	virtual FOnMouseButtonClick& OnItemDoubleClicked() override
	{
		return DoubleClicked;
	}
	virtual FOnContextMenuOpening& OnContextMenuOpened() override
	{
		return ContextMenuOpened;
	}
	virtual TArray<TSharedPtr<FAssetViewItem>> GetSelectedItems() override
	{
		return Selected;
	}
	virtual void SetSelection(const TSharedPtr<FAssetViewItem>& Item, bool bSelected, ESelectInfo::Type Info) override;
	virtual void ClearSelection() override;
	virtual void FocusList() override;
	virtual void RequestScrollIntoView(const TSharedPtr<FAssetViewItem>& Item) override;
	virtual bool IsRightClickScrolling() override;
	virtual FText GetViewDisplayName() override;
	virtual FText GetViewTooltipText() override;

protected:
	virtual void DrawImGuiContent() override;

private:
	struct FHitItem
	{
		TWeakPtr<FAssetViewItem> Item;
		FSlateRect Rect;
		FSlateRect NameRect;
	};
	TSharedPtr<SAssetView> FindOwner();
	TSharedPtr<FAssetViewItem> ItemAt(FVector2D Position) const;
	void RebuildItems();
	void AssignSelected(const TSharedPtr<FAssetViewItem>& Item, bool bSelected);
	void SelectIndex(int32 Index, bool bToggle, bool bRange, ESelectInfo::Type Info = ESelectInfo::OnMouseClick);
	void ProcessItem(int32 Index, bool bClicked, const FSlateRect& NameRect);
	void DrawItem(int32 Index, bool bTile);
	void DrawList();
	void DrawGrid();
	void HandleNavigation();
	void HandleBackgroundClick();
	void ShowContextMenu();
	void ShowRenameEditor(TWeakPtr<FAssetViewItem> Item);
	void DrawSortControls();
	void SaveLayout();
	void ShowThumbnailEditor(const TSharedPtr<FAssetViewItem>& Item);

	TArray<TSharedPtr<FAssetViewItem>>* ItemsSource = nullptr;
	TArray<TSharedPtr<FAssetViewItem>> Items;
	TArray<TSharedPtr<FAssetViewItem>> Selected;
	TArray<TPair<FName, FText>> AvailableColumns;
	TSet<FName> VisibleColumns;
	FString SettingsSection;
	TMap<const FAssetViewItem*, FName> SelectedPaths;
	TArray<FHitItem> HitItems;
	TArray<TFunction<void()>> PendingActions;
	TWeakPtr<SAssetView> Owner;
	TWeakPtr<FAssetViewItem> Anchor;
	TWeakPtr<FAssetViewItem> NavigationItem;
	TWeakPtr<FAssetViewItem> DragSourceItem;
	TWeakPtr<FAssetViewItem> DragHoverItem;
	double DragHoverStart = 0.0;
	TWeakPtr<FAssetViewItem> PendingScroll;
	TWeakPtr<FAssetViewItem> ActiveRename;
	TWeakPtr<FAssetViewItem> ToolTipItem;
	TSharedPtr<SWidget> ToolTipSource;
	FTBImGuiInlineRename Rename;
	FTBImGuiEditorScroll Scroll;
	TUniquePtr<FTBImGuiAssetThumbnails> Thumbnails;
	FOnSelectionChanged SelectionChanged;
	FOnItemScrolledIntoView ScrolledIntoView;
	FOnMouseButtonClick DoubleClicked;
	FOnContextMenuOpening ContextMenuOpened;
	bool bItemsDirty = true;
	bool bSettingsLoaded = false;
	bool bRenameDrawn = false;
	bool bGrid = true;
	bool bThemeApplied = false;
	float TileSize = 112.0f;
	bool bSaveThumbnailSize = false;
	float GridWrapWidth = 0.0f;
	float GridLabelHeight = 0.0f;
	float GridFontSize = 0.0f;
	bool bGridLabelsDirty = true;
	int32 GridColumns = 1;
};
