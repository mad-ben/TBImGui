// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Widgets/SLeafWidget.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
THIRD_PARTY_INCLUDES_END

#include "TBImGuiContext.h"

#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
#define TBIMGUI_DRAWBUFFERS 2
#endif

enum class ETBMouseButtonEvent : uint8
{
	Pressed,
	Released,
	DoubleClick
};

/// @brief ImGui overlay widget used to catch contextual input for ImGui and render it via Slate.
class TBIMGUI_API SImGuiWidget : public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SImGuiWidget)
		: _IsOwnedByApp(true)
		, _Context(nullptr)
		, _InitialContentSize(FVector2D::ZeroVector)
		, _OnTick()
		{}
		SLATE_ARGUMENT(bool, IsOwnedByApp);
		SLATE_ARGUMENT(TSharedPtr<FTBImGuiContext>, Context);
		SLATE_ARGUMENT(FVector2D, InitialContentSize);
		SLATE_ARGUMENT(FSimpleDelegate, OnTick)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	virtual ~SImGuiWidget() override;
	
	/// Begin SLeafWidget Override
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	/// End SLeafWidget Override
	
	/// Begin SWidget Override
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& Event) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& Event) override;
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& Event) override;
	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& Event) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& Event) override;
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& FocusEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;
	/// End SWidget Override

	/// @brief Returns the owning ImGui context for this overlay.
	TSharedPtr<FTBImGuiContext> GetOwningContext() const;
	
	/// @brief Enable/Disable input for this widget while it is visible.
	void SetInputEnabled(bool bEnableInput);
	
	/// @brief Returns true if this widget can currently detect input. False otherwise.
	bool IsInputEnabled() const;
	
	/// @brief Deep-copies the active frames draw data to be used in the OnPaint function.
	void UpdateDrawData(ImDrawData* InDrawData);
	
	/// @brief Set the ImGui content size.
	void SetContentSize(const FVector2D& InContentSize);
	
	/// @brief Return the last cached content size for this widget.
	FVector2D GetContentSize() const;
	
protected:
	
	/// Begin SWidget Override
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	/// End SWidget Override
	
	/// @brief Should ImGui handle an input key event?
	virtual bool ShouldHandleKeyEvent(const FKeyEvent& InEvent) const;
	
	/// @brief Handle any Key event and pass it through to the ImGui input layer.
	virtual FReply HandleKeyEvent(const FKeyEvent& InEvent, bool bKeyDown);
	
	/// @brief Handle any Mouse Button event and pass it through to the ImGui input layer.
	virtual FReply HandleMouseButtonEvent(const FPointerEvent& InEvent, const ETBMouseButtonEvent InMouseButtonEvent);
	
	/// @brief Ran to update the visible state of this widget depending on the input state.
	void UpdateVisibility();
	
	bool bInputEnabled = false;
	bool bIsOwnedByApp = false;
	
	FSimpleDelegate OnTickEvent;
	
	TSharedPtr<FTBImGuiContext> OwningContext = nullptr;
	
private:

#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
	/// Custom Slate drawing element. Used to improve drawing performance.
	/// This is multi-buffered because it is read and written simultaneously from the render and the game threads.
	int32 CurrentDrawBuffer = 0;
	uint64 DrawFrameCounter = 0;
	TSharedPtr<class FTBImGuiCustomElement> ImGuiDrawers[TBIMGUI_DRAWBUFFERS];
	ImDrawData* CachedDrawData = nullptr;
#elif IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_SLATE  
	struct FTBImGuiDrawData
	{
		~FTBImGuiDrawData()
		{
			Empty();
		}
	
		void Update(const ImDrawData* InDrawData);
		void Empty();
		
		ImVec2 DisplayPos;
		ImVec2 DisplaySize;
		TArray<ImDrawList*> DrawLists;
	};
	
	FTBImGuiDrawData CachedDrawData;
#endif
	
	FVector2D ContentSize = FVector2D::ZeroVector;
	
	int16 HideCursorCount = 0;
};