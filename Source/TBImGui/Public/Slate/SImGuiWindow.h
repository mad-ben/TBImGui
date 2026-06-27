// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Widgets/SWindow.h>

struct ImGuiViewport;

/// @brief ImGui window implementation to host ImGui viewports.
class TBIMGUI_API SImGuiWindow : public SWindow
{
public:
	
	SLATE_BEGIN_ARGS(SImGuiWindow)
		: _InitialOpacity(1.0f)
		, _IsTopmostWindow(false)
		{}
		SLATE_ARGUMENT(ImGuiViewport*, Viewport);
		SLATE_ARGUMENT(float, InitialOpacity);
		SLATE_ARGUMENT(bool, IsTopmostWindow)
		SLATE_DEFAULT_SLOT(FArguments, Content);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& Args);
	
	/// Begin SWindow Override
	virtual bool OnIsActiveChanged(const FWindowActivateEvent& ActivateEvent) override;
	/// End SWindow Override
	
	/// @brief Set the windows' topmost state.
	void SetTopMost(bool bIsTopMost);
};