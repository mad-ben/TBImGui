// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IPropertyHandle;
struct ImGuiInputTextCallbackData;

class FTBImGuiPropertyEditor
{
public:
	static bool Supports(const TSharedPtr<IPropertyHandle>& Handle);
	TOptional<FString> Draw(const TSharedRef<IPropertyHandle>& Handle);
	static bool Apply(const TSharedRef<IPropertyHandle>& Handle, const FString& Value);
	bool IsEditing() const
	{
		return bEditing;
	}
	FString Error;

private:
	static int ResizeText(ImGuiInputTextCallbackData* Data);
	void SetText(const FString& Text);
	TArray<ANSICHAR> Buffer;
	bool bEditing = false;
};
