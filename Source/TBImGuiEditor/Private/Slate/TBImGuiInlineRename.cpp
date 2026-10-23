// Copyright Lewis Scrivens. All Rights Reserved.
#include "Slate/TBImGuiInlineRename.h"

#include "Slate/SImGuiWidget.h"
#include "Layout/Children.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

FTBImGuiInlineRename::~FTBImGuiInlineRename()
{
	Cancel();
}

TSharedPtr<SInlineEditableTextBlock> FTBImGuiInlineRename::FindEditor(const TSharedRef<SWidget>& Widget)
{
	if (Widget->GetType() == FName(TEXT("SInlineEditableTextBlock")))
	{
		return StaticCastSharedRef<SInlineEditableTextBlock>(Widget);
	}
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
	{
		if (const auto Editor = FindEditor(Children->GetChildAt(Index)))
		{
			return Editor;
		}
	}
	return nullptr;
}

bool FTBImGuiInlineRename::Begin(const TSharedRef<SWidget>& Widget)
{
	Cancel();
	const auto Editor = FindEditor(Widget);
	if (!Editor || !Editor->BeginExternalEditing())
	{
		return false;
	}
	NativeWidget = Widget;
	NativeEditor = Editor;
	OriginalText = Editor->GetText();
	FCStringAnsi::Strncpy(Buffer, TCHAR_TO_UTF8(*OriginalText.ToString()), UE_ARRAY_COUNT(Buffer));
	Error = FText::GetEmpty();
	bFocus = true;
	return true;
}

bool FTBImGuiInlineRename::IsActive() const
{
	return NativeEditor.IsValid() && !bCommitPending;
}

void FTBImGuiInlineRename::Draw(float Width)
{
	if (!IsActive())
	{
		return;
	}
	ImGui::SetNextItemWidth(FMath::Max(32.0f, Width));
	if (bFocus)
	{
		ImGui::SetKeyboardFocusHere();
		bFocus = false;
	}
	const bool bEnter = ImGui::InputText("##Rename", Buffer, UE_ARRAY_COUNT(Buffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
	const bool bEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);
	const bool bDeactivated = ImGui::IsItemDeactivated();
	const FText NewText = FText::FromString(UTF8_TO_TCHAR(Buffer));
	Error = FText::GetEmpty();
	const bool bValid = NativeEditor->VerifyExternalText(NewText, Error);
	if (!Error.IsEmpty() && (ImGui::IsItemActive() || ImGui::IsItemHovered()))
	{
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(TCHAR_TO_UTF8(*Error.ToString()));
		ImGui::EndTooltip();
	}
	if (bEscape)
	{
		PendingCommit = ETextCommit::OnCleared;
		bCommitPending = true;
	}
	else if (bEnter || bDeactivated)
	{
		if (bValid)
		{
			PendingCommit = bEnter ? ETextCommit::OnEnter : ETextCommit::OnUserMovedFocus;
			bCommitPending = true;
		}
		else if (bEnter)
		{
			bFocus = true;
		}
		else
		{
			PendingCommit = ETextCommit::OnCleared;
			bCommitPending = true;
		}
	}
}

void FTBImGuiInlineRename::Flush()
{
	if (bCommitPending && NativeEditor)
	{
		const auto Editor = MoveTemp(NativeEditor);
		const auto Widget = MoveTemp(NativeWidget);
		(void)Widget;
		bCommitPending = false;
		const FText Text = PendingCommit == ETextCommit::OnCleared ? OriginalText : FText::FromString(UTF8_TO_TCHAR(Buffer));
		Editor->CommitExternalText(Text, PendingCommit);
	}
}

void FTBImGuiInlineRename::Cancel()
{
	if (NativeEditor)
	{
		PendingCommit = ETextCommit::OnCleared;
		bCommitPending = true;
		Flush();
	}
}
