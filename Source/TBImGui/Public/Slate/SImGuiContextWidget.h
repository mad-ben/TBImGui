// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include "SImGuiWidget.h"

/// @brief Self-contained ImGui widget that owns its own context and draws content internally.
class TBIMGUI_API SImGuiContextWidget : public SImGuiWidget
{
public:
	
	SLATE_BEGIN_ARGS(SImGuiContextWidget)
		: _ContextParams()
		, _OwningWindow(nullptr)
		, _DrawContent()
		{}
		SLATE_ARGUMENT(FTBImGuiContextParams, ContextParams)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, OwningWindow)
		SLATE_ARGUMENT(FSimpleDelegate, DrawContent)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
	/// Begin SImGuiWidget Override
	/// @brief Internally tick our own content.
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	/// End SImGuiWidget Override

protected:
	
	/// @brief Called during this contexts frame to draw ImGui content.
	virtual void DrawImGuiContent();
};