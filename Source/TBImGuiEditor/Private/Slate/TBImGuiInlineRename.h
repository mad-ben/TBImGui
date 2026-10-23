// Copyright Lewis Scrivens. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"

class SInlineEditableTextBlock;
class SWidget;

class FTBImGuiInlineRename
{
public:
	~FTBImGuiInlineRename();
	bool Begin(const TSharedRef<SWidget>& Widget);
	void Draw(float Width);
	void Flush();
	void Cancel();
	bool IsActive() const;
	static TSharedPtr<SInlineEditableTextBlock> FindEditor(const TSharedRef<SWidget>& Widget);

private:
	TSharedPtr<SWidget> NativeWidget;
	TSharedPtr<SInlineEditableTextBlock> NativeEditor;
	FText OriginalText;
	FText Error;
	ANSICHAR Buffer[4096] = {};
	ETextCommit::Type PendingCommit = ETextCommit::Default;
	bool bCommitPending = false;
	bool bFocus = false;
};
