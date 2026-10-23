// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/TBImGuiPropertyEditor.h"

#include "PropertyHandle.h"
#include "imgui.h"
#include "String/LexFromString.h"
#include "imgui_internal.h"
#include "Algo/AllOf.h"

bool FTBImGuiPropertyEditor::Supports(const TSharedPtr<IPropertyHandle>& Handle)
{
	const FProperty* Property = Handle && Handle->IsValidHandle() ? Handle->GetProperty() : nullptr;
	return Property && (Property->IsA<FBoolProperty>() || Property->IsA<FNumericProperty>() || Property->IsA<FEnumProperty>() || Property->IsA<FStrProperty>() || Property->IsA<FNameProperty>()) && !Handle->HasMetaData(TEXT("Bitmask")) && !Handle->HasMetaData(TEXT("MultiLine")) && !Handle->HasMetaData(TEXT("PasswordField"));
}

void FTBImGuiPropertyEditor::SetText(const FString& Text)
{
	const FTCHARToUTF8 Converted(*Text);
	Buffer.SetNumUninitialized(FMath::Max(Converted.Length() + 1, 128));
	FMemory::Memcpy(Buffer.GetData(), Converted.Get(), Converted.Length());
	Buffer[Converted.Length()] = '\0';
}

int FTBImGuiPropertyEditor::ResizeText(ImGuiInputTextCallbackData* Data)
{
	FTBImGuiPropertyEditor& Editor = *static_cast<FTBImGuiPropertyEditor*>(Data->UserData);
	Editor.Buffer.SetNumUninitialized(Data->BufSize);
	Data->Buf = Editor.Buffer.GetData();
	return 0;
}

TOptional<FString> FTBImGuiPropertyEditor::Draw(const TSharedRef<IPropertyHandle>& Handle)
{
	FProperty* Property = Handle->GetProperty();
	if (Property->IsA<FBoolProperty>())
	{
		bool bValue = false;
		const FPropertyAccess::Result Result = Handle->GetValue(bValue);
		ImGui::BeginDisabled(Result == FPropertyAccess::Fail);
		if (Result == FPropertyAccess::MultipleValues)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
		}
		const bool bChanged = ImGui::Checkbox("##Value", &bValue);
		if (Result == FPropertyAccess::MultipleValues)
		{
			ImGui::PopItemFlag();
		}
		ImGui::EndDisabled();
		return bChanged ? TOptional<FString>(bValue ? TEXT("True") : TEXT("False")) : TOptional<FString>();
	}

	FString Value;
	const FPropertyAccess::Result Result = Handle->GetValueAsFormattedString(Value);
	if (Property->IsA<FStrProperty>())
	{
		Handle->GetValue(Value);
	}
	else if (Property->IsA<FNameProperty>())
	{
		FName Name;
		Handle->GetValue(Name);
		Value = Name.ToString();
	}
	if (Result == FPropertyAccess::Fail)
	{
		ImGui::TextDisabled("Unavailable");
		return {};
	}

	const FByteProperty* Byte = CastField<FByteProperty>(Property);
	if (Property->IsA<FEnumProperty>() || (Byte && Byte->Enum))
	{
		FString DisplayValue;
		Handle->GetValueAsDisplayString(DisplayValue);
		if (Result == FPropertyAccess::MultipleValues)
		{
			DisplayValue = TEXT("Multiple Values");
		}
		TOptional<FString> Selection;
		if (ImGui::BeginCombo("##Value", TCHAR_TO_UTF8(*DisplayValue)))
		{
			TArray<FString> Options;
			TArray<FText> ToolTips;
			TArray<bool> Restricted;
			TArray<FText> DisplayNames;
			Handle->GeneratePossibleValues(Options, ToolTips, Restricted, &DisplayNames);
			for (int32 Index = 0; Index < Options.Num(); ++Index)
			{
				ImGui::PushID(Index);
				const FString Label = DisplayNames.IsValidIndex(Index) ? DisplayNames[Index].ToString() : Options[Index];
				ImGui::BeginDisabled(Restricted.IsValidIndex(Index) && Restricted[Index]);
				if (ImGui::Selectable(TCHAR_TO_UTF8(*Label), Result == FPropertyAccess::Success && Options[Index] == Value))
				{
					Selection = Options[Index];
				}
				ImGui::EndDisabled();
				if (ToolTips.IsValidIndex(Index) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				{
					ImGui::SetTooltip("%s", TCHAR_TO_UTF8(*ToolTips[Index].ToString()));
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		return Selection;
	}

	if (!bEditing)
	{
		SetText(Result == FPropertyAccess::MultipleValues ? FString() : Value);
	}
	const bool bCancelled = bEditing && ImGui::IsKeyPressed(ImGuiKey_Escape);
	const bool bEnter = ImGui::InputTextWithHint("##Value", Result == FPropertyAccess::MultipleValues ? "Multiple Values" : "", Buffer.GetData(), Buffer.Num(),
	                                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize, &FTBImGuiPropertyEditor::ResizeText, this);
	const bool bCommit = bEnter || ImGui::IsItemDeactivatedAfterEdit();
	bEditing = ImGui::IsItemActive();
	if (bCommit && !bCancelled)
	{
		const FString NewValue(UTF8_TO_TCHAR(Buffer.GetData()));
		if (Result == FPropertyAccess::MultipleValues || NewValue != Value)
		{
			return NewValue;
		}
	}
	return {};
}

bool FTBImGuiPropertyEditor::Apply(const TSharedRef<IPropertyHandle>& Handle, const FString& Value)
{
	if (!Handle->IsValidHandle() || Handle->IsEditConst() || Handle->IsRestricted(Value))
	{
		return false;
	}
	FProperty* Property = Handle->GetProperty();
	if (!Property)
	{
		return false;
	}
	if (Property->IsA<FStrProperty>())
	{
		return Handle->SetValue(Value) == FPropertyAccess::Success;
	}
	if (Property->IsA<FNameProperty>())
	{
		if (Value.Len() >= NAME_SIZE)
		{
			return false;
		}
		return Handle->SetValue(FName(*Value)) == FPropertyAccess::Success;
	}
	if (const FNumericProperty* Numeric = CastField<FNumericProperty>(Property); Numeric && !Numeric->IsEnum())
	{
		double Number = 0.0;
		if (!LexTryParseString(Number, *Value) || !FMath::IsFinite(Number))
		{
			return false;
		}
		if (Numeric->IsInteger())
		{
			int64 SignedValue = 0;
			uint64 UnsignedValue = 0;
			const bool bUnsigned = Property->IsA<FByteProperty>() || Property->IsA<FUInt16Property>() || Property->IsA<FUInt32Property>() || Property->IsA<FUInt64Property>();
			if (bUnsigned ? (Value.StartsWith(TEXT("-")) || !LexTryParseString(UnsignedValue, *Value) || !Numeric->CanHoldValue(UnsignedValue))
			              : (!LexTryParseString(SignedValue, *Value) || !Numeric->CanHoldValue(SignedValue)))
			{
				return false;
			}
			const FString Digits = Value.Mid(Value.StartsWith(TEXT("-")) || Value.StartsWith(TEXT("+")) ? 1 : 0);
			if (Digits.IsEmpty() || !Algo::AllOf(Digits, [](TCHAR Character)
			                                     { return FChar::IsDigit(Character); }))
			{
				return false;
			}
		}
		else if (Property->IsA<FFloatProperty>() && FMath::Abs(Number) > TNumericLimits<float>::Max())
		{
			return false;
		}
		if ((Handle->HasMetaData(TEXT("ClampMin")) && Number < Handle->GetDoubleMetaData(TEXT("ClampMin"))) || (Handle->HasMetaData(TEXT("ClampMax")) && Number > Handle->GetDoubleMetaData(TEXT("ClampMax"))))
		{
			return false;
		}
	}
	return Handle->SetValueFromFormattedString(Value) == FPropertyAccess::Success;
}
