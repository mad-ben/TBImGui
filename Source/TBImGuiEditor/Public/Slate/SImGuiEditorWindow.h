// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Widgets/SWindow.h>

#include <TBImGuiContext.h>

/// @brief ImGui content based editor window.
class TBIMGUIEDITOR_API SImGuiEditorWindow : public SWindow
{
public:
	
	SLATE_BEGIN_ARGS(SImGuiEditorWindow)
		: _Title(FText::GetEmpty())
		, _InitialSize(FVector2f(512.0f))
		, _DrawContent()
		{}
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(FVector2f, InitialSize)	
		SLATE_ARGUMENT(FSimpleDelegate, DrawContent)	
	SLATE_END_ARGS()
	
	void Construct(const FArguments& Args);
	
	/// Begin SWindow Override
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	/// End SWindow Override
	
	/// @brief Return the editor windows ImGui context.
	TSharedRef<FTBImGuiContext> GetWindowContext();
	
protected:
	
	/// @brief Draw the ImGui content during the WindowContext frame.
	virtual void DrawImGuiContent();
	
	TSharedPtr<FTBImGuiContext> WindowContext = nullptr;
	
	FSimpleDelegate OnDrawContent;
};