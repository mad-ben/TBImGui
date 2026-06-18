// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiContextWidget.h"

void SImGuiContextWidget::Construct(const FArguments& InArgs)
{
	// Create custom ImGui context for tab.
	TSharedRef<FTBImGuiContext> NewContext = FTBImGuiContext::CreateContext(InArgs._ContextParams);
	FTBImGuiContextScope ContextScope(NewContext);
	check(ContextScope.IsValid());
	
	// Bind this widget as the main viewport and set-up owning window.
	FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::FindOrCreate(ImGui::GetMainViewport());
	check(ViewportData);
	ViewportData->Window = InArgs._OwningWindow;
	ViewportData->ImGuiWidget = SharedThis(this);
	
	// Construct the base ImGui widget to use a self-contained context.
	SImGuiWidget::Construct(
		SImGuiWidget::FArguments()
		.IsOwnedByApp(false)
		.Context(NewContext)
		.OnTick(InArgs._DrawContent));
}

void SImGuiContextWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	if (ContextScope.IsValid())
	{
		ContextScope->BeginFrame();
		DrawImGuiContent();
		ContextScope->EndFrame();
	}
}

void SImGuiContextWidget::DrawImGuiContent()
{
	OnTickEvent.ExecuteIfBound();
}