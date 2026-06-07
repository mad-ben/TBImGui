// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include "SImGuiWidget.h"

#include <Engine/GameInstance.h>

/// @brief ImGui widget overlay for a runtime game viewport.
class TBIMGUI_API SImGuiViewportWidget : public SImGuiWidget
{
public:
	
	SLATE_BEGIN_ARGS(SImGuiViewportWidget)
		: _IsOwnedByApp(true)
		, _Context(nullptr)
		, _GameInstance(nullptr)
		{}
		SLATE_ARGUMENT(bool, IsOwnedByApp);
		SLATE_ARGUMENT(TSharedPtr<FTBImGuiContext>, Context);
		SLATE_ARGUMENT(TObjectPtr<UGameInstance>, GameInstance);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
	/// Begin SImGuiOverlay Override
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	/// Begin SImGuiOverlay Override
	
	/// @brief Set the input passthrough state for the mouse.
	/// @note If true, mouse input will be marked unhandled and passed through the widget to the game viewport.
	/// @note If false, mouse input will always be caught by the viewport overlay if input is enabled.
	void SetMouseInputPassthrough(bool bPassthrough);

	/// @brief Set the input passthrough state for the mouse.
	/// @note If true, keyboard input will be marked unhandled and passed through the widget to the game viewport.
	/// @note If false, keyboard input will always be caught by the viewport overlay if input is enabled.
	void SetKeyboardInputPassthrough(bool bPassthrough);
	
	/// @brief Disable all input passthrough.
	/// @note Can be useful for temporarily disabling passthrough.
	void SetDisableInputPassthrough(bool bDisablePassthrough);
	
	/// @brief Is the mouse input marked to pass through this widget?
	bool CanMousePassthrough() const;

	/// @brief Is the keyboard input marked to pass through this widget?
	bool CanKeyboardPassthrough() const;
	
protected:
	
	/// Begin SImGuiOverlay Override
	virtual FReply HandleKeyEvent(const FKeyEvent& InEvent, bool bKeyDown) override;
	virtual FReply HandleMouseButtonEvent(const FPointerEvent& InEvent, const ETBMouseButtonEvent InMouseButtonEvent) override;
	virtual bool ShouldHandleKeyEvent(const FKeyEvent& InEvent) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	/// End SImGuiOverlay Override
	
private:
	
	TWeakObjectPtr<UGameInstance> GameInstanceWeak = nullptr;
	
	bool bMouseInputPassthrough = true;
	bool bKeyboardInputPassthrough = true;
	bool bDisableInputPassthrough = false;
};