// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiViewportWidget.h"

#include <GameFramework/InputSettings.h>

#include "TBImGuiTypes.h"

void SImGuiViewportWidget::Construct(const FArguments& InArgs)
{
	checkf(IsValid(InArgs._GameInstance), TEXT("Trying to construct ImGui viewport widget with invalid context!"));
	GameInstanceWeak = InArgs._GameInstance;
	
	SImGuiWidget::FArguments ParentArgs;
	ParentArgs._IsOwnedByApp = InArgs._IsOwnedByApp;
	ParentArgs._Context = InArgs._Context;
	SImGuiWidget::Construct(ParentArgs);
}

void SImGuiViewportWidget::SetMouseInputPassthrough(bool bPassthrough)
{
	bMouseInputPassthrough = bPassthrough;
}

void SImGuiViewportWidget::SetKeyboardInputPassthrough(bool bPassthrough)
{
	bKeyboardInputPassthrough = bPassthrough;
}

void SImGuiViewportWidget::SetDisableInputPassthrough(bool bDisablePassthrough)
{
	bDisableInputPassthrough = bDisablePassthrough;
}

bool SImGuiViewportWidget::CanMousePassthrough() const
{
	return !bDisableInputPassthrough && bMouseInputPassthrough;
}

bool SImGuiViewportWidget::CanKeyboardPassthrough() const
{
	return !bDisableInputPassthrough && bKeyboardInputPassthrough;
}

FReply SImGuiViewportWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	if (!IO.WantCaptureMouse && CanMousePassthrough())
	{
		return FReply::Unhandled();
	}
	
	return SImGuiWidget::OnMouseWheel(MyGeometry, MouseEvent);
}

FReply SImGuiViewportWidget::HandleKeyEvent(const FKeyEvent& InEvent, bool bKeyDown)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	// Only process passthrough for keyboard input.
	// Otherwise, gamepad navigation will be passed through the viewport and remain unprocessed.
	if (!InEvent.GetKey().IsGamepadKey())
	{
		ImGuiIO& IO = ImGui::GetIO();
		if (!IO.WantCaptureKeyboard && CanKeyboardPassthrough())
		{
			return FReply::Unhandled();
		}
	}
	
	return SImGuiWidget::HandleKeyEvent(InEvent, bKeyDown);
}

FReply SImGuiViewportWidget::HandleMouseButtonEvent(const FPointerEvent& InEvent, const ETBMouseButtonEvent InMouseButtonEvent)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	if (!IO.WantCaptureMouse && CanMousePassthrough())
	{
		return FReply::Unhandled();
	}
	
	return SImGuiWidget::HandleMouseButtonEvent(InEvent, InMouseButtonEvent);
}

bool SImGuiViewportWidget::ShouldHandleKeyEvent(const FKeyEvent& InEvent) const
{
	if (!SImGuiWidget::ShouldHandleKeyEvent(InEvent))
	{
		return false;
	}
	
	// If our game viewport does not have user focus, allow all key events.
	{
		bool bAppHasFocus;
		if (GEngine && IsValid(GEngine->GameViewport) && GEngine->GameViewport->Viewport)
		{
			bAppHasFocus = GEngine->GameViewport->Viewport->IsForegroundWindow();
		}
		else
		{
			bAppHasFocus = FApp::HasFocus();
		}
		
		if (!bAppHasFocus)
		{
			return true;
		}
	}
	
	// If it's the end PIE session key, do not handle it.
	TSharedPtr<FUICommandInfo> StopPieCommand = FInputBindingManager::Get().FindCommandInContext("PlayWorld", "StopPlaySession");
	if (StopPieCommand.IsValid())
	{
		const FInputChord InputChord = FInputChord(InEvent.GetKey(), InEvent.IsShiftDown(), InEvent.IsControlDown(), InEvent.IsAltDown(), InEvent.IsCommandDown());
		if (StopPieCommand->HasActiveChord(InputChord))
		{
			return false;
		}
	}

	const bool bControlPressed = InEvent.IsControlDown();
	const bool bShiftPressed = InEvent.IsShiftDown();
	const bool bAltPressed = InEvent.IsAltDown();
	const bool bCmdPressed = InEvent.IsCommandDown();
	
	// If it's bound to open the console, do not handle it.
	if (!bControlPressed && !bShiftPressed && !bAltPressed && !bCmdPressed)
	{
		const UInputSettings* const InputSettings = GetDefault<UInputSettings>();
		if (IsValid(InputSettings) && InputSettings->ConsoleKeys.Contains(InEvent.GetKey()))
		{
			return false;
		}
	}
	
	// If it's bound to any existing commands for this viewports player controller, do not handle it.
	if (GameInstanceWeak.IsValid())
	{
		APlayerController* const LocalPlayerController = GameInstanceWeak->GetFirstLocalPlayerController();
		if (IsValid(LocalPlayerController) && IsValid(LocalPlayerController->PlayerInput))
		{
			for (const FKeyBind& KeyBind : LocalPlayerController->PlayerInput->DebugExecBindings)
			{
				// Taken from UPlayerInput::GetBind
				const FKey Key = InEvent.GetKey();
				if (KeyBind.Key == Key && !KeyBind.bDisabled)
				{
					if ((!KeyBind.Control || bControlPressed) && (!KeyBind.Shift || bShiftPressed) && (!KeyBind.Alt || bAltPressed) && (!KeyBind.Cmd || bCmdPressed)
						&&	(!KeyBind.bIgnoreCtrl || !bControlPressed) && (!KeyBind.bIgnoreShift || !bShiftPressed) && (!KeyBind.bIgnoreAlt || !bAltPressed) && (!KeyBind.bIgnoreCmd || !bCmdPressed))
					{
						return false;
					}
				}
			}
		}
	}
	
	return true;
}

FVector2D SImGuiViewportWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	// Returning zero so content is auto-scaled to the viewport.
	return FVector2D::ZeroVector;
}