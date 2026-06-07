// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiWindow.h"

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
THIRD_PARTY_INCLUDES_END

void SImGuiWindow::Construct(const FArguments& Args)
{
	const bool bIsTooltip = (ImGui::GetWindowFlags(Args._Viewport) & ImGuiWindowFlags_Tooltip);
	const bool bTopMost = (Args._Viewport->Flags & ImGuiViewportFlags_TopMost) || Args._IsTopmostWindow;
	const bool bPopupWindow = (Args._Viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon);
	const bool bNoFocusOnAppearing = (Args._Viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing);
	
	static FWindowStyle WindowStyle = FWindowStyle()
		.SetActiveTitleBrush(FSlateNoResource())
		.SetInactiveTitleBrush(FSlateNoResource())
		.SetFlashTitleBrush(FSlateNoResource())
		.SetOutlineBrush(FSlateNoResource())
		.SetBorderBrush(FSlateNoResource())
		.SetBackgroundBrush(FSlateNoResource())
		.SetChildBackgroundBrush(FSlateNoResource())
		.SetCornerRadius(0.0f);
	
	SWindow::Construct(
		SWindow::FArguments()
		.Type(bIsTooltip ? EWindowType::ToolTip : EWindowType::Menu)
		.Style(&WindowStyle)
		.ScreenPosition(FVector2f(Args._Viewport->Pos))
		.ClientSize(FVector2f(Args._Viewport->Size))
		.SupportsTransparency(EWindowTransparency::PerWindow)
		.InitialOpacity(Args._InitialOpacity)
		.SizingRule(ESizingRule::Autosized)
		.IsPopupWindow(bIsTooltip || bPopupWindow)
		.IsTopmostWindow(bTopMost)
		.FocusWhenFirstShown(!bNoFocusOnAppearing)
		.ActivationPolicy(bNoFocusOnAppearing ? EWindowActivationPolicy::Never : EWindowActivationPolicy::Always)
		.AdjustInitialSizeAndPositionForDPIScale(false)
		.HasCloseButton(false)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.CreateTitleBar(false)
		.LayoutBorder(FMargin(0.0f))
		.UserResizeBorder(FMargin(0.0f))
		.MinWidth(1.0f)
		.MinHeight(1.0f)
		.UseOSWindowBorder(false)
		[
			Args._Content.Widget
		]
	);
}

bool SImGuiWindow::OnIsActiveChanged(const FWindowActivateEvent& ActivateEvent)
{
	// Force mouse activation to be handled as normal so focus is processed.
	if (ActivateEvent.GetActivationType() == FWindowActivateEvent::EA_ActivateByMouse)
	{
		const FWindowActivateEvent ModifiedActivateEvent(FWindowActivateEvent::EA_Activate, ActivateEvent.GetAffectedWindow());
		return SWindow::OnIsActiveChanged(ModifiedActivateEvent);
	}
	
	return SWindow::OnIsActiveChanged(ActivateEvent);
}

void SImGuiWindow::SetTopMost(bool bIsTopMost)
{
	bIsTopmostWindow = bIsTopMost;
}