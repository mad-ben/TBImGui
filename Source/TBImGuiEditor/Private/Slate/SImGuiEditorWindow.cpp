// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiEditorWindow.h"

#include <TBImGuiUtils.h>

void SImGuiEditorWindow::Construct(const FArguments& Args)
{
	// Create slate window.
	SWindow::Construct(
		SWindow::FArguments()
		.Title(Args._Title)
		.ClientSize(Args._InitialSize)
		.AutoCenter(EAutoCenter::None)
		.SizingRule(ESizingRule::UserSized)
		.SupportsMaximize(true)
		.SupportsMinimize(true));
	
	OnDrawContent = Args._DrawContent;
	
	// Add ImGui context to the example window.
	FTBImGuiContextParams ContextParams;
	ContextParams.ContextName = TEXT("EditorWindow");
	ContextParams.ViewportMode = ETBImGuiViewportMode::Single;
	WindowContext = FTBImGuiUtils::CreateWindowContext(SharedThis(this), ContextParams);
	
	ForceVolatile(true);
	SetCanTick(true);
}

void SImGuiEditorWindow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SWindow::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	
	FTBImGuiContextScope ContextScope(WindowContext);
	if (ContextScope.IsValid())
	{
		ContextScope->BeginFrame();
		DrawImGuiContent();
		ContextScope->EndFrame();
	}
}

TSharedRef<FTBImGuiContext> SImGuiEditorWindow::GetWindowContext() const
{
	return WindowContext.ToSharedRef();
}

void SImGuiEditorWindow::DrawImGuiContent()
{
	OnDrawContent.ExecuteIfBound();
}