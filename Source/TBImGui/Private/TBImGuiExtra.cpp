// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiExtra.h"

#include <InputCoreTypes.h>
#include <HAL/PlatformApplicationMisc.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
#include <imgui_internal.h>
THIRD_PARTY_INCLUDES_END

#include "TBImGuiUtils.h"
#include "TBImGuiTypes.h"
#include "Fonts/IconsFontAwesome.h"
#include "Slate/SImGuiWindow.h"
#include "Slate/SImGuiWidget.h"

#pragma region Conversion

FColor ImGui::ToFColor(const uint32 InColor)
{
	return FColor(InColor);
}

FLinearColor ImGui::ToFLinearColor(const uint32 InColor)
{
	return FLinearColor(FColor(InColor));
}

ImColor ImGui::ToImColor(const FColor& InColor)
{
	return ImColor(InColor.R, InColor.G, InColor.B, InColor.A);
}

ImU32 ImGui::ToImU32(const FColor& InColor)
{
	return ToImColor(InColor);
}

ImColor ImGui::ToImColor(const FLinearColor& InColor)
{
	return ImColor(InColor.R, InColor.G, InColor.B, InColor.A);
}

ImVec4 ImGui::ToImVec4(const FColor& InColor)
{
	return ToImColor(InColor).Value;
}

ImVec4 ImGui::ToImVec4(const FLinearColor& InColor)
{
	return ImVec4(InColor.R, InColor.G, InColor.B, InColor.A);
}

ImGuiKey ImGui::ToImGuiKey(const FKey& InKey)
{
	static const TMap<FKey, ImGuiKey> KeyLookupMap =
	{
		{ EKeys::Tab, ImGuiKey_Tab },

		{ EKeys::Left, ImGuiKey_LeftArrow },
		{ EKeys::Right, ImGuiKey_RightArrow },
		{ EKeys::Up, ImGuiKey_UpArrow },
		{ EKeys::Down, ImGuiKey_DownArrow },

		{ EKeys::PageUp, ImGuiKey_PageUp },
		{ EKeys::PageDown, ImGuiKey_PageDown },
		{ EKeys::Home, ImGuiKey_Home },
		{ EKeys::End, ImGuiKey_End },
		{ EKeys::Insert, ImGuiKey_Insert },
		{ EKeys::Delete, ImGuiKey_Delete },

		{ EKeys::BackSpace, ImGuiKey_Backspace },
		{ EKeys::SpaceBar, ImGuiKey_Space },
		{ EKeys::Enter, ImGuiKey_Enter },
		{ EKeys::Escape, ImGuiKey_Escape },

		{ EKeys::LeftControl, ImGuiKey_LeftCtrl },
		{ EKeys::LeftShift, ImGuiKey_LeftShift },
		{ EKeys::LeftAlt, ImGuiKey_LeftAlt },
		{ EKeys::LeftCommand, ImGuiKey_LeftSuper },
		{ EKeys::RightControl, ImGuiKey_RightCtrl },
		{ EKeys::RightShift, ImGuiKey_RightShift },
		{ EKeys::RightAlt, ImGuiKey_RightAlt },
		{ EKeys::RightCommand, ImGuiKey_RightSuper },

		{ EKeys::Zero, ImGuiKey_0 },
		{ EKeys::One, ImGuiKey_1 },
		{ EKeys::Two, ImGuiKey_2 },
		{ EKeys::Three, ImGuiKey_3 },
		{ EKeys::Four, ImGuiKey_4 },
		{ EKeys::Five, ImGuiKey_5 },
		{ EKeys::Six, ImGuiKey_6 },
		{ EKeys::Seven, ImGuiKey_7 },
		{ EKeys::Eight, ImGuiKey_8 },
		{ EKeys::Nine, ImGuiKey_9 },

		{ EKeys::A, ImGuiKey_A },
		{ EKeys::B, ImGuiKey_B },
		{ EKeys::C, ImGuiKey_C },
		{ EKeys::D, ImGuiKey_D },
		{ EKeys::E, ImGuiKey_E },
		{ EKeys::F, ImGuiKey_F },
		{ EKeys::G, ImGuiKey_G },
		{ EKeys::H, ImGuiKey_H },
		{ EKeys::I, ImGuiKey_I },
		{ EKeys::J, ImGuiKey_J },
		{ EKeys::K, ImGuiKey_K },
		{ EKeys::L, ImGuiKey_L },
		{ EKeys::M, ImGuiKey_M },
		{ EKeys::N, ImGuiKey_N },
		{ EKeys::O, ImGuiKey_O },
		{ EKeys::P, ImGuiKey_P },
		{ EKeys::Q, ImGuiKey_Q },
		{ EKeys::R, ImGuiKey_R },
		{ EKeys::S, ImGuiKey_S },
		{ EKeys::T, ImGuiKey_T },
		{ EKeys::U, ImGuiKey_U },
		{ EKeys::V, ImGuiKey_V },
		{ EKeys::W, ImGuiKey_W },
		{ EKeys::X, ImGuiKey_X },
		{ EKeys::Y, ImGuiKey_Y },
		{ EKeys::Z, ImGuiKey_Z },

		{ EKeys::F1, ImGuiKey_F1 },
		{ EKeys::F2, ImGuiKey_F2 },
		{ EKeys::F3, ImGuiKey_F3 },
		{ EKeys::F4, ImGuiKey_F4 },
		{ EKeys::F5, ImGuiKey_F5 },
		{ EKeys::F6, ImGuiKey_F6 },
		{ EKeys::F7, ImGuiKey_F7 },
		{ EKeys::F8, ImGuiKey_F8 },
		{ EKeys::F9, ImGuiKey_F9 },
		{ EKeys::F10, ImGuiKey_F10 },
		{ EKeys::F11, ImGuiKey_F11 },
		{ EKeys::F12, ImGuiKey_F12 },

		{ EKeys::Apostrophe, ImGuiKey_Apostrophe },
		{ EKeys::Comma, ImGuiKey_Comma },
		{ EKeys::Hyphen, ImGuiKey_Minus },
		{ EKeys::Period, ImGuiKey_Period },
		{ EKeys::Slash, ImGuiKey_Slash },
		{ EKeys::Semicolon, ImGuiKey_Semicolon },
		{ EKeys::Equals, ImGuiKey_Equal },
		{ EKeys::LeftBracket, ImGuiKey_LeftBracket },
		{ EKeys::Backslash, ImGuiKey_Backslash },
		{ EKeys::RightBracket, ImGuiKey_RightBracket },
		{ EKeys::Tilde, ImGuiKey_GraveAccent },

		{ EKeys::CapsLock, ImGuiKey_CapsLock },
		{ EKeys::ScrollLock, ImGuiKey_ScrollLock },
		{ EKeys::NumLock, ImGuiKey_NumLock },
		{ EKeys::Pause, ImGuiKey_Pause },

		{ EKeys::NumPadZero, ImGuiKey_Keypad0 },
		{ EKeys::NumPadOne, ImGuiKey_Keypad1 },
		{ EKeys::NumPadTwo, ImGuiKey_Keypad2 },
		{ EKeys::NumPadThree, ImGuiKey_Keypad3 },
		{ EKeys::NumPadFour, ImGuiKey_Keypad4 },
		{ EKeys::NumPadFive, ImGuiKey_Keypad5 },
		{ EKeys::NumPadSix, ImGuiKey_Keypad6 },
		{ EKeys::NumPadSeven, ImGuiKey_Keypad7 },
		{ EKeys::NumPadEight, ImGuiKey_Keypad8 },
		{ EKeys::NumPadNine, ImGuiKey_Keypad9 },

		{ EKeys::Decimal, ImGuiKey_KeypadDecimal },
		{ EKeys::Divide, ImGuiKey_KeypadDivide },
		{ EKeys::Multiply, ImGuiKey_KeypadMultiply },
		{ EKeys::Subtract, ImGuiKey_KeypadSubtract },
		{ EKeys::Add, ImGuiKey_KeypadAdd },
		
		// Unreal doesn't have separation between equal and numpad equal.
		// { EKeys::Equals, ImGuiKey_KeypadEqual },

		{ EKeys::Gamepad_Special_Right, ImGuiKey_GamepadStart },
		{ EKeys::Gamepad_Special_Left, ImGuiKey_GamepadBack },
		{ EKeys::Gamepad_FaceButton_Left, ImGuiKey_GamepadFaceLeft },
		{ EKeys::Gamepad_FaceButton_Right, ImGuiKey_GamepadFaceRight },
		{ EKeys::Gamepad_FaceButton_Top, ImGuiKey_GamepadFaceUp },
		{ EKeys::Gamepad_FaceButton_Bottom, ImGuiKey_GamepadFaceDown },
		{ EKeys::Gamepad_DPad_Left, ImGuiKey_GamepadDpadLeft },
		{ EKeys::Gamepad_DPad_Right, ImGuiKey_GamepadDpadRight },
		{ EKeys::Gamepad_DPad_Up, ImGuiKey_GamepadDpadUp },
		{ EKeys::Gamepad_DPad_Down, ImGuiKey_GamepadDpadDown },
		{ EKeys::Gamepad_LeftShoulder, ImGuiKey_GamepadL1 },
		{ EKeys::Gamepad_RightShoulder, ImGuiKey_GamepadR1 },
		{ EKeys::Gamepad_LeftTrigger, ImGuiKey_GamepadL2 },
		{ EKeys::Gamepad_RightTrigger, ImGuiKey_GamepadR2 },
		{ EKeys::Gamepad_LeftThumbstick, ImGuiKey_GamepadL3 },
		{ EKeys::Gamepad_RightThumbstick, ImGuiKey_GamepadR3 },
		{ EKeys::Gamepad_LeftStick_Left, ImGuiKey_GamepadLStickLeft },
		{ EKeys::Gamepad_LeftStick_Right, ImGuiKey_GamepadLStickRight },
		{ EKeys::Gamepad_LeftStick_Up, ImGuiKey_GamepadLStickUp },
		{ EKeys::Gamepad_LeftStick_Down, ImGuiKey_GamepadLStickDown },
		{ EKeys::Gamepad_RightStick_Left, ImGuiKey_GamepadRStickLeft },
		{ EKeys::Gamepad_RightStick_Right, ImGuiKey_GamepadRStickRight },
		{ EKeys::Gamepad_RightStick_Up, ImGuiKey_GamepadRStickUp },
		{ EKeys::Gamepad_RightStick_Down, ImGuiKey_GamepadRStickDown }
	};

	const ImGuiKey* Result = KeyLookupMap.Find(InKey);
	return (Result != nullptr) ? *Result : ImGuiKey_None;
}

ImGuiMouseButton_ ImGui::ToImGuiMouseButton(const FKey& InKey)
{
	static const TMap<FKey, ImGuiMouseButton_> MouseButtonLookupMap =
	{
		{ EKeys::LeftMouseButton, ImGuiMouseButton_Left },
		{ EKeys::RightMouseButton, ImGuiMouseButton_Right },
		{ EKeys::MiddleMouseButton, ImGuiMouseButton_Middle },
	};

	const ImGuiMouseButton_* Result = MouseButtonLookupMap.Find(InKey);
	return (Result != nullptr) ? *Result : ImGuiMouseButton_COUNT;
}

FKey ImGui::ToUnrealFKey(const ImGuiKey InKey)
{
	static const TMap<ImGuiKey, FKey> KeyLookupMap =
	{
		{ ImGuiKey_Tab, EKeys::Tab },

		{ ImGuiKey_LeftArrow, EKeys::Left },
		{ ImGuiKey_RightArrow, EKeys::Right },
		{ ImGuiKey_UpArrow, EKeys::Up },
		{ ImGuiKey_DownArrow, EKeys::Down },

		{ ImGuiKey_PageUp, EKeys::PageUp },
		{ ImGuiKey_PageDown, EKeys::PageDown },
		{ ImGuiKey_Home, EKeys::Home },
		{ ImGuiKey_End, EKeys::End },
		{ ImGuiKey_Insert, EKeys::Insert },
		{ ImGuiKey_Delete, EKeys::Delete },

		{ ImGuiKey_Backspace, EKeys::BackSpace },
		{ ImGuiKey_Space, EKeys::SpaceBar },
		{ ImGuiKey_Enter, EKeys::Enter },
		{ ImGuiKey_Escape, EKeys::Escape },

		{ ImGuiKey_LeftCtrl, EKeys::LeftControl },
		{ ImGuiKey_LeftShift, EKeys::LeftShift },
		{ ImGuiKey_LeftAlt, EKeys::LeftAlt },
		{ ImGuiKey_LeftSuper, EKeys::LeftCommand },
		{ ImGuiKey_RightCtrl, EKeys::RightControl },
		{ ImGuiKey_RightShift, EKeys::RightShift },
		{ ImGuiKey_RightAlt, EKeys::RightAlt },
		{ ImGuiKey_RightSuper, EKeys::RightCommand },

		{ ImGuiKey_0, EKeys::Zero },
		{ ImGuiKey_1, EKeys::One },
		{ ImGuiKey_2, EKeys::Two },
		{ ImGuiKey_3, EKeys::Three },
		{ ImGuiKey_4, EKeys::Four },
		{ ImGuiKey_5, EKeys::Five },
		{ ImGuiKey_6, EKeys::Six },
		{ ImGuiKey_7, EKeys::Seven },
		{ ImGuiKey_8, EKeys::Eight },
		{ ImGuiKey_9, EKeys::Nine },

		{ ImGuiKey_A, EKeys::A },
		{ ImGuiKey_B, EKeys::B },
		{ ImGuiKey_C, EKeys::C },
		{ ImGuiKey_D, EKeys::D },
		{ ImGuiKey_E, EKeys::E },
		{ ImGuiKey_F, EKeys::F },
		{ ImGuiKey_G, EKeys::G },
		{ ImGuiKey_H, EKeys::H },
		{ ImGuiKey_I, EKeys::I },
		{ ImGuiKey_J, EKeys::J },
		{ ImGuiKey_K, EKeys::K },
		{ ImGuiKey_L, EKeys::L },
		{ ImGuiKey_M, EKeys::M },
		{ ImGuiKey_N, EKeys::N },
		{ ImGuiKey_O, EKeys::O },
		{ ImGuiKey_P, EKeys::P },
		{ ImGuiKey_Q, EKeys::Q },
		{ ImGuiKey_R, EKeys::R },
		{ ImGuiKey_S, EKeys::S },
		{ ImGuiKey_T, EKeys::T },
		{ ImGuiKey_U, EKeys::U },
		{ ImGuiKey_V, EKeys::V },
		{ ImGuiKey_W, EKeys::W },
		{ ImGuiKey_X, EKeys::X },
		{ ImGuiKey_Y, EKeys::Y },
		{ ImGuiKey_Z, EKeys::Z },

		{ ImGuiKey_F1, EKeys::F1 },
		{ ImGuiKey_F2, EKeys::F2 },
		{ ImGuiKey_F3, EKeys::F3 },
		{ ImGuiKey_F4, EKeys::F4 },
		{ ImGuiKey_F5, EKeys::F5 },
		{ ImGuiKey_F6, EKeys::F6 },
		{ ImGuiKey_F7, EKeys::F7 },
		{ ImGuiKey_F8, EKeys::F8 },
		{ ImGuiKey_F9, EKeys::F9 },
		{ ImGuiKey_F10, EKeys::F10 },
		{ ImGuiKey_F11, EKeys::F11 },
		{ ImGuiKey_F12, EKeys::F12 },

		{ ImGuiKey_Apostrophe, EKeys::Apostrophe },
		{ ImGuiKey_Comma, EKeys::Comma },
		{ ImGuiKey_Minus, EKeys::Hyphen },
		{ ImGuiKey_Period, EKeys::Period },
		{ ImGuiKey_Slash, EKeys::Slash },
		{ ImGuiKey_Semicolon, EKeys::Semicolon },
		{ ImGuiKey_Equal, EKeys::Equals },
		{ ImGuiKey_LeftBracket, EKeys::LeftBracket },
		{ ImGuiKey_Backslash, EKeys::Backslash },
		{ ImGuiKey_RightBracket, EKeys::RightBracket },
		{ ImGuiKey_GraveAccent, EKeys::Tilde },

		{ ImGuiKey_CapsLock, EKeys::CapsLock },
		{ ImGuiKey_ScrollLock, EKeys::ScrollLock },
		{ ImGuiKey_NumLock, EKeys::NumLock },
		{ ImGuiKey_Pause, EKeys::Pause },

		{ ImGuiKey_Keypad0, EKeys::NumPadZero },
		{ ImGuiKey_Keypad1, EKeys::NumPadOne },
		{ ImGuiKey_Keypad2, EKeys::NumPadTwo },
		{ ImGuiKey_Keypad3, EKeys::NumPadThree },
		{ ImGuiKey_Keypad4, EKeys::NumPadFour },
		{ ImGuiKey_Keypad5, EKeys::NumPadFive },
		{ ImGuiKey_Keypad6, EKeys::NumPadSix },
		{ ImGuiKey_Keypad7, EKeys::NumPadSeven },
		{ ImGuiKey_Keypad8, EKeys::NumPadEight },
		{ ImGuiKey_Keypad9, EKeys::NumPadNine },

		{ ImGuiKey_KeypadDecimal, EKeys::Decimal },
		{ ImGuiKey_KeypadDivide, EKeys::Divide },
		{ ImGuiKey_KeypadMultiply, EKeys::Multiply },
		{ ImGuiKey_KeypadSubtract, EKeys::Subtract },
		{ ImGuiKey_KeypadAdd, EKeys::Add },
		
		// Unreal doesn't have separation between equal and numpad equal.
		// { ImGuiKey_KeypadEqual, EKeys::Equals },

		{ ImGuiKey_GamepadStart, EKeys::Gamepad_Special_Right },
		{ ImGuiKey_GamepadBack, EKeys::Gamepad_Special_Left },
		{ ImGuiKey_GamepadFaceLeft, EKeys::Gamepad_FaceButton_Left },
		{ ImGuiKey_GamepadFaceRight, EKeys::Gamepad_FaceButton_Right },
		{ ImGuiKey_GamepadFaceUp, EKeys::Gamepad_FaceButton_Top },
		{ ImGuiKey_GamepadFaceDown, EKeys::Gamepad_FaceButton_Bottom },
		{ ImGuiKey_GamepadDpadLeft, EKeys::Gamepad_DPad_Left },
		{ ImGuiKey_GamepadDpadRight, EKeys::Gamepad_DPad_Right },
		{ ImGuiKey_GamepadDpadUp, EKeys::Gamepad_DPad_Up },
		{ ImGuiKey_GamepadDpadDown, EKeys::Gamepad_DPad_Down },
		{ ImGuiKey_GamepadL1, EKeys::Gamepad_LeftShoulder },
		{ ImGuiKey_GamepadR1, EKeys::Gamepad_RightShoulder },
		{ ImGuiKey_GamepadL2, EKeys::Gamepad_LeftTrigger },
		{ ImGuiKey_GamepadR2, EKeys::Gamepad_RightTrigger },
		{ ImGuiKey_GamepadL3, EKeys::Gamepad_LeftThumbstick },
		{ ImGuiKey_GamepadR3, EKeys::Gamepad_RightThumbstick },
		{ ImGuiKey_GamepadLStickLeft, EKeys::Gamepad_LeftStick_Left },
		{ ImGuiKey_GamepadLStickRight, EKeys::Gamepad_LeftStick_Right },
		{ ImGuiKey_GamepadLStickUp, EKeys::Gamepad_LeftStick_Up },
		{ ImGuiKey_GamepadLStickDown, EKeys::Gamepad_LeftStick_Down },
		{ ImGuiKey_GamepadRStickLeft, EKeys::Gamepad_RightStick_Left },
		{ ImGuiKey_GamepadRStickRight, EKeys::Gamepad_RightStick_Right },
		{ ImGuiKey_GamepadRStickUp, EKeys::Gamepad_RightStick_Up },
		{ ImGuiKey_GamepadRStickDown, EKeys::Gamepad_RightStick_Down }
	};

	const FKey* const Result = KeyLookupMap.Find(InKey);
	return (Result != nullptr) ? *Result : FKey();
}

FKey ImGui::ToUnrealFKey_Mouse(const ImGuiMouseButton_ InKey)
{
	static const TMap<ImGuiMouseButton_, FKey> MouseButtonLookupMap =
	{
		{ ImGuiMouseButton_Left, EKeys::LeftMouseButton },
		{ ImGuiMouseButton_Right, EKeys::RightMouseButton },
		{ ImGuiMouseButton_Middle, EKeys::MiddleMouseButton },
	};

	const FKey* Result = MouseButtonLookupMap.Find(InKey);
	return (Result != nullptr) ? *Result : FKey();
}

ImGuiKeyChord ImGui::ToImGuiKeyChord(const FInputChord& InChord)
{
	ImGuiKeyChord Result = ToImGuiKey(InChord.Key);
	
	Result |= InChord.bCtrl ? ImGuiMod_Ctrl : 0;
	Result |= InChord.bShift ? ImGuiMod_Shift : 0;
	Result |= InChord.bAlt ? ImGuiMod_Alt : 0;
	Result |= InChord.bCmd ? ImGuiMod_Super : 0;
	
	return Result;
}

FInputChord ImGui::ToUnrealFInputChord(const ImGuiKeyChord InChord)
{
	FInputChord Result = ToUnrealFKey(static_cast<ImGuiKey>(InChord & ~ImGuiMod_Mask_));
	
	Result.bCtrl = (InChord & ImGuiMod_Ctrl) != 0;
	Result.bShift = (InChord & ImGuiMod_Shift) != 0;
	Result.bAlt = (InChord & ImGuiMod_Alt) != 0;
	Result.bCmd = (InChord & ImGuiMod_Super) != 0;

	return Result;
}

EMouseCursor::Type ImGui::ToUnrealMouseCursor(const ImGuiMouseCursor InMouseCursor)
{
	static const TMap<ImGuiMouseCursor, EMouseCursor::Type> MouseCursorLookupMap =
	{
		{ ImGuiMouseCursor_None, EMouseCursor::None },
		{ ImGuiMouseCursor_Arrow, EMouseCursor::Default },
		{ ImGuiMouseCursor_TextInput, EMouseCursor::TextEditBeam },
		{ ImGuiMouseCursor_ResizeAll, EMouseCursor::CardinalCross },
		{ ImGuiMouseCursor_ResizeNS, EMouseCursor::ResizeUpDown },
		{ ImGuiMouseCursor_ResizeEW, EMouseCursor::ResizeLeftRight },
		{ ImGuiMouseCursor_ResizeNESW, EMouseCursor::ResizeSouthWest },
		{ ImGuiMouseCursor_ResizeNWSE, EMouseCursor::ResizeSouthEast },
		{ ImGuiMouseCursor_Hand, EMouseCursor::GrabHand },
		{ ImGuiMouseCursor_NotAllowed, EMouseCursor::SlashedCircle }
	};
	
	const EMouseCursor::Type* Result = MouseCursorLookupMap.Find(InMouseCursor);
	return (Result != nullptr) ? *Result : EMouseCursor::None;
}

#pragma endregion

#pragma region Text

void ImGui::TextSelectable(const char* InText)
{
	PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, 0.0f});
	PushStyleColor(ImGuiCol_FrameBg, {0.0f, 0.0f, 0.0f, 0.0f});

	PushID(InText);
	{
		if (!InText)
		{
			InText = "";
		}

		constexpr size_t BufferSize = 1024;
		const size_t InLength = std::strlen(InText);
		const size_t CopyLength = InLength < BufferSize - 1 ? InLength : BufferSize - 1;

		char Buffer[BufferSize] = {};
		if (CopyLength > 0)
		{
			FMemory::Memcpy(Buffer, InText, CopyLength);
		}
		
		Buffer[CopyLength] = '\0';
		
		InputTextEx("", nullptr, Buffer, IM_COUNTOF(Buffer),
			CalcTextSize(Buffer), ImGuiInputTextFlags_ReadOnly);
	}
	PopID();
	
	PopStyleColor();
	PopStyleVar();
}

void ImGui::TextCopy(const char* InText, ...)
{
	char Buffer[1024];
	va_list Args;
	va_start(Args, InText);
	const int32 Length = ImFormatStringV(Buffer, IM_COUNTOF(Buffer), InText, Args);
	va_end(Args);
	
	const char* const TextEnd = Buffer + ImMin(Length, IM_COUNTOF(Buffer) - 1);
	TextUnformatted(Buffer, TextEnd);
	
	SameLine();
	if (Button(ICON_FA_COPY))
	{
		FPlatformApplicationMisc::ClipboardCopy(UTF8_TO_TCHAR(Buffer));
	}
}

void ImGui::TextAligned(const char* InText, ...)
{
	AlignTextToFramePadding();
	va_list Args;
	va_start(Args, InText);
	TextV(InText, Args);
	va_end(Args);
}

void ImGui::TextCentered(const char* InText, ...)
{
	char TextBuffer[1024];
	va_list Args;
	va_start(Args, InText);
	ImFormatStringV(TextBuffer, IM_COUNTOF(TextBuffer), InText, Args);
	va_end(Args);
	
	const float WidthAvail = GetContentRegionAvail().x;
	const float TextWidth = CalcTextSize(TextBuffer).x;
	if (WidthAvail > TextWidth)
	{
		Dummy(ImVec2((WidthAvail - TextWidth) / 2.0f, 0.0f));
		SameLine(0.0f, 0.0f);
	}

	TextUnformatted(TextBuffer);
}

void ImGui::TextTooltip(const char* InText, ...)
{
	PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
	TextUnformatted("(" ICON_FA_INFO ")");
	PopStyleColor();
	va_list Args;
	va_start(Args, InText);
	HoveredTooltip(InText, Args);
	va_end(Args);
}

void ImGui::TextTooltipEx(ImGuiHoveredFlags Flags, const char* InText, ...)
{
	PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
	TextUnformatted("(" ICON_FA_INFO ")");
	PopStyleColor();
	va_list Args;
	va_start(Args, InText);
	HoveredTooltipEx(Flags, InText, Args);
	va_end(Args);
}

#pragma endregion

#pragma region Layout

void ImGui::CenterItem(int32 ItemWidth)
{
	const float WidthAvail = GetContentRegionAvail().x;
	if (WidthAvail > ItemWidth)
	{
		Dummy(ImVec2((WidthAvail - ItemWidth) / 2.0f, 0.0f));
		SameLine(0.0f, 0.0f);
	}
}

void ImGui::SetNextItemSpan()
{
	SetNextItemWidth(GetContentRegionAvail().x);
}

void ImGui::AlignTextToTreeNode()
{
	DummyTreeNode();
	SameLine();
}

void ImGui::DummyTreeNode()
{
	const ImGuiWindow* const Window = GetCurrentWindow();
	ImGuiContext& ImGuiContext = *GImGui;
	const ImGuiStyle& style = ImGuiContext.Style;
	const float LineHeight = ImMax(ImMin(Window->DC.CurrLineSize.y, ImGuiContext.FontSize + style.FramePadding.y * 2), ImGuiContext.FontSize);
	Dummy(ImVec2(ImGuiContext.FontSize, LineHeight));
}

int32 ImGui::GetTreeNodeDepth()
{
	const ImGuiWindow* const Window = GetCurrentWindow();
	return Window->DC.TreeDepth;
}

int32 ImGui::GetIndentationCount()
{
	ImGuiContext& Context = *GImGui;
	if (Context.Style.IndentSpacing == 0)
	{
		return 0;
	}
	
	const ImGuiWindow* const Window = GetCurrentWindow();
	return static_cast<int32>(Window->DC.Indent.x / Context.Style.IndentSpacing);
}

#pragma endregion

#pragma region Interaction

bool ImGui::CheckboxButton(const char* InText, bool& bChecked, TOptional<FColor> InCheckedColor)
{
	int32 StyleCount = 0;
	PushStyleColor(ImGuiCol_Button, ToImU32(FColor::Transparent));
	StyleCount++;

	if (bChecked)
	{
		if (InCheckedColor.IsSet())
		{
			PushStyleColor(ImGuiCol_Text, ToImU32(InCheckedColor.GetValue()));
			StyleCount++;
		}
	}
	else
	{
		PushStyleColor(ImGuiCol_Text, ToImU32(FColor(120,120,120)));
		StyleCount++;
	}

	const bool bPressed = Button(InText);
	if (bPressed)
	{
		bChecked = !bChecked;
	}
	
	PopStyleColor(StyleCount);
	return bPressed;
}

bool ImGui::TextButton(const char* InText)
{
	int32 StyleCount = 0;
	PushStyleColor(ImGuiCol_Button, ToImU32(FColor::Transparent));
	StyleCount++;

	const bool bPressed = Button(InText);
	PopStyleColor(StyleCount);
	return bPressed;
}

bool ImGui::CheckboxSelectable(const char* InText, bool& bCheckedState, ImGuiSelectableFlags InFlags)
{
	if (bCheckedState)
	{
		Text(ICON_FA_CHECK);
	}
	else
	{
		Dummy(ImVec2(CalcTextSize(ICON_FA_CHECK).x, 1.0f));
	}

	SameLine();
	const bool bPressed = Selectable(InText, false, InFlags);
	if (bPressed)
	{
		bCheckedState = !bCheckedState;
	}

	return bPressed;
}

bool ImGui::FColorEdit(const char* InLabel, FColor& OutColor, ImGuiColorEditFlags InFlags)
{
	FLinearColor LinearColor = OutColor;
	
	const bool bEdited = ColorEdit4(InLabel, &LinearColor.R, InFlags | ImGuiColorEditFlags_Uint8);
	if (bEdited)
	{
		OutColor = LinearColor.ToFColor(true);
	}
	
	return bEdited;
}

bool ImGui::FLinearColorEdit(const char* InLabel, FLinearColor& OutColor, ImGuiColorEditFlags InFlags)
{
	return ColorEdit4(InLabel, &OutColor.R, InFlags | ImGuiColorEditFlags_Uint8);
}

bool ImGui::DragDouble(const char* InLabel, double* OutValue, float InSpeed, double InMin, double InMax, const char* InFormat, ImGuiSliderFlags flags)
{
	return DragScalar(InLabel, ImGuiDataType_Double, OutValue, InSpeed, &InMin, &InMax, InFormat, flags);
}

void ImGui::HoveredTooltip(const char* InText, ...)
{
	if (IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_DelayNormal))
	{
		if (BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			va_list Args;
			va_start(Args, InText);
			TextV(InText, Args);
			va_end(Args);
			EndTooltip();
		}
	}
}

void ImGui::HoveredTooltipEx(ImGuiHoveredFlags Flags, const char* InText, ...)
{
	if (IsItemHovered(Flags))
	{
		if (BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			va_list Args;
			va_start(Args, InText);
			TextV(InText, Args);
			va_end(Args);
			EndTooltip();
		}
	}
}

int ImGui::InputTextFilter_UnrealFileSystemTypes(ImGuiInputTextCallbackData* InData)
{
	const FString RestrictedChars = FPaths::GetInvalidFileSystemChars();
	const FString InputChar = FString::Chr(static_cast<TCHAR>(InData->EventChar));
	if (RestrictedChars.Contains(InputChar) || InputChar == " ")
	{
		return 1;
	}

	return 0;
}

#pragma endregion

#pragma region Table

bool ImGui::TableNextRowFirstColumn()
{
	ImGuiContext& Context = *GImGui;
	ImGuiTable* const CurrentTable = Context.CurrentTable;
	if (!CurrentTable)
	{
		return false;
	}
	
	TableNextRow();
	TableBeginCell(CurrentTable, 0);
	return true;
}

bool ImGui::TableGetColumnEnabled(int32 ColumnIndex)
{
	if (ColumnIndex < 0)
	{
		return false;
	}
	
	ImGuiContext& Context = *GImGui;
	ImGuiTable* const CurrentTable = Context.CurrentTable;
	if (!CurrentTable)
	{
		return false;
	}

	if (ColumnIndex >= CurrentTable->ColumnsCount)
	{
		return false;
	}

	ImGuiTableColumn* Column = &CurrentTable->Columns[ColumnIndex];
	return Column->IsUserEnabledNextFrame;
}

void ImGui::TableSetBgColorByOffset(int32 Offset, int32 NumColors, float ColorBlend)
{
	const ImGuiTable* const CurrentTable = GetCurrentTable();
	if (!CurrentTable)
	{
		return;
	}
	
	if (Offset == 0)
	{
		return;
	}

	const int32 ColorIndex = (Offset - 1) % NumColors;
	float Hue = (ColorIndex / (float)NumColors);
	
	ImVec4 CurrentColor = ColorConvertU32ToFloat4(CurrentTable->RowBgColor[1]);
	ImVec4 RowColor = ImVec4(0.0f, 0.0f, 0.0f, 0.05f);
	ColorConvertHSVtoRGB(Hue, 1.0f, 1.0f, RowColor.x, RowColor.y, RowColor.z);

	ImVec4 FinalColor = ImLerp(CurrentColor, RowColor, ColorBlend);
	TableSetBgColor(ImGuiTableBgTarget_RowBg1, GetColorU32(FinalColor));
}

void ImGui::TableSetBgColorByDepth(int32 NumColors, float ColorBlend)
{
	TableSetBgColorByOffset(GetTreeNodeDepth(), NumColors, ColorBlend);
}

void ImGui::TableRowHighlightOnHovered()
{
	const ImGuiTable* const CurrentTable = GetCurrentTable();
	if (!CurrentTable)
	{
		return;
	}
	
	if (TableGetHoveredRow() == CurrentTable->CurrentRow)
	{
		TableSetBgColor(ImGuiTableBgTarget_RowBg1, GetColorU32(ImGuiCol_FrameBgHovered));
	}
}

bool ImGui::TableRowBeginPopupContextItem(const char* InId)
{
	const ImGuiTable* const CurrentTable = GetCurrentTable();
	if (!CurrentTable)
	{
		return false;
	}

	const int32 ActiveRow = CurrentTable->CurrentRow;
	if (TableGetHoveredRow() == ActiveRow)
	{
		if (!IsAnyItemHovered() && IsMouseReleased(ImGuiMouseButton_Right))
		{
			OpenPopup(InId);
		}
	}

	if (BeginPopup(InId))
	{
		return true;
	}

	return false;
}

#pragma endregion 

const char* ImGui::FormatTemp(const char* InText, ...)
{
	static char Buf[256];
	va_list Args;
	va_start(Args, InText);
	ImFormatStringV(Buf, IM_ARRAYSIZE(Buf), InText, Args);
	va_end(Args);
	return Buf;
}

FVector2f ImGui::GetScreenSpaceAbsolutePosition()
{
	if (const FTBImGuiViewportData* const MainViewportData = FTBImGuiViewportData::Find(GetMainViewport()))
	{
		if (MainViewportData->ImGuiWidget.IsValid())
		{
			return MainViewportData->ImGuiWidget.Pin()->GetTickSpaceGeometry().AbsolutePosition;
		}
	}

	return FVector2f::ZeroVector;
}

ImGuiWindow* ImGui::GetWindow(ImGuiViewport* InViewport)
{
	ImGuiViewportP* const ViewportPtr = static_cast<ImGuiViewportP*>(InViewport);
	if (!ViewportPtr)
	{
		return nullptr;
	}
	
	return ViewportPtr->Window;
}

ImGuiWindowFlags ImGui::GetWindowFlags(ImGuiViewport* InViewport)
{
	const ImGuiWindow* const Window = GetWindow(InViewport);
	if (!Window)
	{
		return 0;
	}
	
	return Window->Flags;
}

void ImGui::AddNextWindowFlags(ImGuiWindowFlags InFlagsToAdd)
{
	ImGuiContext& g = *GImGui;
	g.NextWindowData.HasFlags |= InFlagsToAdd;
}

void ImGui::RemoveNextWindowFlags(ImGuiWindowFlags InFlagsToRemove)
{
	ImGuiContext& g = *GImGui;
	g.NextWindowData.HasFlags &= ~InFlagsToRemove;
}

void ImGui::SetWindowTopmost(const ImGuiWindow* InWindow, bool bTopMost)
{
	if (!InWindow || !InWindow->Viewport)
	{
		return;
	}
	
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InWindow->Viewport))
	{
		const TSharedPtr<SImGuiWindow> FoundWindow = StaticCastSharedPtr<SImGuiWindow>(ViewportData->Window.Pin());
		if (FoundWindow.IsValid())
		{
			FoundWindow->SetTopMost(bTopMost);
		}
	}
	
	if (bTopMost)
	{
		InWindow->Viewport->Flags |= ImGuiViewportFlags_TopMost;
	}
	else
	{
		InWindow->Viewport->Flags &= ~ImGuiViewportFlags_TopMost;
	}
}

bool ImGui::IsForegroundWindow(const ImGuiWindow* InWindow)
{
	ImGuiContext& Context = *GImGui;
	return Context.PlatformIO.Platform_GetWindowFocus(InWindow->Viewport);
}

bool ImGui::IsComboOpen(const char* ComboID)
{
	ImGuiID ComboPopupID = ImHashStr("##ComboPopup", 0, GetID(ComboID));
	if (IsPopupOpen(ComboPopupID, 0))
	{
		return true;
	}

	return false;
}

FString ImGui::ConfigDir()
{
	return FPaths::GeneratedConfigDir() / FPlatformProperties::PlatformName();
}

void ImGui::ClearAllWindowSettings()
{
	ImGuiContext& Context = *GImGui;
	for (ImGuiStoragePair& WindowData : Context.WindowsById.Data)
	{
		if (const ImGuiWindow* const Window = static_cast<ImGuiWindow*>(WindowData.val_p))
		{
			ClearWindowSettings(Window->Name);
		}
	}
	
	for (ImGuiWindowSettings* Settings = Context.SettingsWindows.begin(); Settings != nullptr; Settings = Context.SettingsWindows.next_chunk(Settings))
	{
		if (Settings)
		{
			Settings->WantDelete = true;
		}
	}
}

void ImGui::SaveIniSettings()
{
	const ImGuiContext& Context = *GImGui;
	SaveIniSettingsToDisk(TCHAR_TO_UTF8(*FTBImGuiUtils::GetImGuiConfigPath(FString(Context.ContextName))));
}

void ImGui::LoadIniSettings()
{
	const ImGuiContext& Context = *GImGui;
	LoadIniSettingsFromDisk(TCHAR_TO_UTF8(*FTBImGuiUtils::GetImGuiConfigPath(FString(Context.ContextName))));
}

void ImGui::FlushInput()
{
	ImGuiIO& IO = GetIO();
	IO.ClearInputKeys();
	IO.ClearInputMouse();
}

void ImGui::ForEachViewport(const TFunctionRef<bool(ImGuiViewportP&)>& InFunction)
{
	ImGuiContext& Context = *GImGui;
	if (Context.Viewports.Size <= 0)
	{
		return;
	}
	
	for (ImGuiViewportP* const Viewport : Context.Viewports)
	{
		if (!Viewport)
		{
			continue;
		}
		
		if (!InFunction(*Viewport))
		{
			return;
		}
	}
}

void ImGui::ForEachWindow(const TFunctionRef<bool(ImGuiWindow&)>& InFunction)
{
	ImGuiContext& Context = *GImGui;
	if (Context.Windows.Size <= 0)
	{
		return;
	}
	
	for (ImGuiWindow* const Window : Context.Windows)
	{
		if (!Window)
		{
			continue;
		}
		
		if (!InFunction(*Window))
		{
			return;
		}
	}
}