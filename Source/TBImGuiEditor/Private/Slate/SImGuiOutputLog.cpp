// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiOutputLog.h"

#include "HAL/CriticalSection.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"
#include "TBImGuiEditor.h"

namespace TBIMGUI_OUTPUT_LOG
{
	constexpr int32 MaxOutputLogLines = 10000;
}

class FTBImGuiOutputLogDevice final : public FOutputDevice
{
public:
	virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		const FString Formatted = FString::Printf(TEXT("[%s] %s"), *Category.ToString(), Message);
		FScopeLock Lock(&CriticalSection);
		PendingLines.Add(Formatted);
	}

	virtual bool CanBeUsedOnAnyThread() const override
	{
		return true;
	}

	virtual bool CanBeUsedOnMultipleThreads() const override
	{
		return true;
	}

	void Drain(TArray<FString>& OutLines)
	{
		FScopeLock Lock(&CriticalSection);
		OutLines.Append(MoveTemp(PendingLines));
		PendingLines.Reset();
	}

private:
	FCriticalSection CriticalSection;
	TArray<FString> PendingLines;
};

void SImGuiOutputLog::Construct(const FArguments& InArgs)
{
	FTBImGuiContextParams ContextParams(TEXT("EditorOutputLog"));
	ContextParams.ViewportMode = ETBImGuiViewportMode::Single;
	ContextParams.bDockingSupport = false;

	SImGuiContextWidget::Construct(
		SImGuiContextWidget::FArguments()
		.ContextParams(ContextParams));

	LogDevice = MakeUnique<FTBImGuiOutputLogDevice>();
	if (GLog)
	{
		GLog->AddOutputDevice(LogDevice.Get());
	}
}

SImGuiOutputLog::~SImGuiOutputLog()
{
	if (GLog && LogDevice.IsValid())
	{
		GLog->RemoveOutputDevice(LogDevice.Get());
	}
}

void SImGuiOutputLog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SetContentSize(AllottedGeometry.GetLocalSize());
	SImGuiContextWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SImGuiOutputLog::RebuildVisibleText()
{
	FString CombinedText;
	const FString FilterText = UTF8_TO_TCHAR(Filter);

	for (const FString& Line : Lines)
	{
		if (FilterText.IsEmpty() || Line.Contains(FilterText, ESearchCase::IgnoreCase))
		{
			CombinedText.Append(Line);
			CombinedText.AppendChar(TEXT('\n'));
		}
	}

	FTCHARToUTF8 ConvertedText(*CombinedText);
	VisibleText.SetNumUninitialized(ConvertedText.Length() + 1);
	FMemory::Memcpy(VisibleText.GetData(), ConvertedText.Get(), ConvertedText.Length());
	VisibleText.Last() = '\0';
	bVisibleTextDirty = false;
}

int SImGuiOutputLog::HandleTextInput(ImGuiInputTextCallbackData* Data)
{
	SImGuiOutputLog* const OutputLog = static_cast<SImGuiOutputLog*>(Data->UserData);
	if (OutputLog && OutputLog->bScrollToEnd)
	{
		Data->CursorPos = Data->BufTextLen;
		Data->SelectionStart = Data->CursorPos;
		Data->SelectionEnd = Data->CursorPos;
		OutputLog->bScrollToEnd = false;
	}

	return 0;
}

void SImGuiOutputLog::DrawImGuiContent()
{
	if (!bThemeApplied)
	{
		FTBImGuiEditorModule::ApplyThemeToCurrentContext();
		bThemeApplied = true;
	}

	if (LogDevice.IsValid())
	{
		const int32 PreviousLineCount = Lines.Num();
		LogDevice->Drain(Lines);
		if (Lines.Num() > TBIMGUI_OUTPUT_LOG::MaxOutputLogLines)
		{
			Lines.RemoveAt(0, Lines.Num() - TBIMGUI_OUTPUT_LOG::MaxOutputLogLines, EAllowShrinking::No);
		}

		if (Lines.Num() != PreviousLineCount)
		{
			bVisibleTextDirty = true;
			bScrollToEnd = bAutoScroll;
		}
	}

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->Pos);
	ImGui::SetNextWindowSize(Viewport->Size);
	ImGui::SetNextWindowViewport(Viewport->ID);

	constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoBringToFrontOnFocus;

	if (ImGui::Begin("##TBImGuiOutputLog", nullptr, WindowFlags))
	{
		if (ImGui::Button("Clear"))
		{
			Lines.Reset();
			bVisibleTextDirty = true;
		}
		ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &bAutoScroll);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputTextWithHint("##Filter", "Filter log output...", Filter, UE_ARRAY_COUNT(Filter)))
		{
			bVisibleTextDirty = true;
		}

		ImGui::Separator();
		if (bVisibleTextDirty)
		{
			RebuildVisibleText();
		}

		const ImGuiInputTextFlags TextFlags = ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CallbackAlways;
		ImGui::InputTextMultiline(
			"##LogText",
			VisibleText.GetData(),
			VisibleText.Num(),
			ImVec2(-1.0f, -1.0f),
			TextFlags,
			&HandleTextInput,
			this);
	}
	ImGui::End();
}
