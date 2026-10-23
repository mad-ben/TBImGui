// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Slate/SImGuiContextWidget.h>

class FTBImGuiOutputLogDevice;

/** A real editor Output Log surface rendered entirely with Dear ImGui. */
class SImGuiOutputLog final : public SImGuiContextWidget
{
public:
	SLATE_BEGIN_ARGS(SImGuiOutputLog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SImGuiOutputLog() override;

	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

protected:
	virtual void DrawImGuiContent() override;

private:
	void RebuildVisibleText();
	static int HandleTextInput(ImGuiInputTextCallbackData* Data);

	TUniquePtr<FTBImGuiOutputLogDevice> LogDevice;
	TArray<FString> Lines;
	TArray<ANSICHAR> VisibleText;
	ANSICHAR Filter[256] = {};
	bool bAutoScroll = true;
	bool bThemeApplied = false;
	bool bVisibleTextDirty = true;
	bool bScrollToEnd = false;
};
