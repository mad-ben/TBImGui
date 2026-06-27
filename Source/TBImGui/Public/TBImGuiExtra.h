// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#ifndef IMGUI_USER_H_FILENAME // Fix for IDE not supporting pre-definition in ImGuiConfig.
#include <imgui.h>
#endif

struct FInputChord;
struct FKey;

struct ImGuiWindow;
struct ImGuiViewport;
struct ImGuiViewportP;

typedef int ImGuiKeyChord;

namespace EMouseCursor
{
	enum Type : int;
}

#define IMGUI_ID_SCOPE(ID)	\
	ImGui::PushID(ID);		\
	ON_SCOPE_EXIT			\
	{						\
		ImGui::PopID();		\
	}

#define IMGUI_DISABLE_SCOPE()	\
	ImGui::BeginDisabled(true);	\
	ON_SCOPE_EXIT				\
	{							\
		ImGui::EndDisabled();	\
	}

#define IMGUI_DISABLE_CONDITION_SCOPE(Condition)	\
	if (Condition)									\
	{												\
		ImGui::BeginDisabled(true);					\
	}												\
	ON_SCOPE_EXIT									\
	{												\
		if (Condition)								\
		{											\
			ImGui::EndDisabled();					\
		}											\
	}												\

#define IMGUI_INDENT_SCOPE()	\
	ImGui::Indent();			\
	ON_SCOPE_EXIT				\
	{							\
		ImGui::Unindent();		\
	}

#define IMGUI_UNINDENT_SCOPE()	\
	ImGui::Unindent();			\
	ON_SCOPE_EXIT				\
	{							\
		ImGui::Indent();		\
	}

namespace ImGui
{
#pragma region Conversion
	
	/// @brief Converts ImGui 32-bit color types to Unreal FColor type.
	TBIMGUI_API FColor ToFColor(const uint32 InColor);
	
	/// @brief Converts ImGui 32-bit color types to Unreal FLinearColor type.
	TBIMGUI_API FLinearColor ToFLinearColor(const uint32 InColor);
	
	/// @brief Converts Unreal FColor type to ImGui ImColor type.
	TBIMGUI_API ImColor ToImColor(const FColor& InColor);

	/// @brief Converts Unreal FColor type to ImGui ImU32 type.
	TBIMGUI_API ImU32 ToImU32(const FColor& InColor);

	/// @brief Converts Unreal FLinearColor type to ImGui ImColor type.
	TBIMGUI_API ImColor ToImColor(const FLinearColor& InColor);

	/// @brief Converts Unreal FColor type to ImGui ImVec4 type.
	TBIMGUI_API ImVec4 ToImVec4(const FColor& InColor);

	/// @brief Converts Unreal FLinearColor type to ImGui ImVec4 type.
	TBIMGUI_API ImVec4 ToImVec4(const FLinearColor& InColor);
	
	/// @brief Converts Unreal FKey type to ImGuiKey type.
	TBIMGUI_API ImGuiKey ToImGuiKey(const FKey& InKey);

	/// @brief Converts Unreal FKey type to ImGuiMouseButton_ type.
	TBIMGUI_API ImGuiMouseButton_ ToImGuiMouseButton(const FKey& InKey);

	/// @brief Converts ImGuiKey type to Unreal FKey type.
	TBIMGUI_API FKey ToUnrealFKey(const ImGuiKey InKey);

	/// @brief Converts ImGuiMouseButton_ type to Unreal FKey type.
	TBIMGUI_API FKey ToUnrealFKey_Mouse(const ImGuiMouseButton_ InKey);
	
	/// @brief Converts Unreal FInputChord type to ImGuiKeyChord type.
	TBIMGUI_API ImGuiKeyChord ToImGuiKeyChord(const FInputChord& InChord);
	
	/// @brief Converts ImGuiKeyChord type to Unreal FInputChord type.
	TBIMGUI_API FInputChord ToUnrealFInputChord(const ImGuiKeyChord InChord);

	/// @brief Converts ImGuiMouseCursor_ type to Unreal EMouseCursor type.
	TBIMGUI_API EMouseCursor::Type ToUnrealMouseCursor(const ImGuiMouseCursor InMouseCursor);
	
#pragma endregion 
	
#pragma region Text
	
	/// @brief Draws text similar to ImGui::Text that can be selected and copied.
	TBIMGUI_API void TextSelectable(const char* InText);

	/// @brief Draws text with copy button at the end.
	TBIMGUI_API void TextCopy(const char* InText, ...) IM_FMTARGS(1);

	/// @brief Draws text after calling ImGui::AlignTextToFramePadding.
	/// @note Use this when drawing text in horizontal columns to align it with the adjacent widgets vertically.
	TBIMGUI_API void TextAligned(const char* InText, ...) IM_FMTARGS(1);

	/// @brief Draws text in the center of the available width in the current cell.
	TBIMGUI_API void TextCentered(const char* InText, ...) IM_FMTARGS(1);

	/// @brief Draws the given tooltip under a text icon if the last item was hovered.
	TBIMGUI_API void TextTooltip(const char* InText, ...) IM_FMTARGS(1);
	
	/// @brief Draws the given tooltip under a text icon if the last item was hovered.
	TBIMGUI_API void TextTooltipEx(ImGuiHoveredFlags Flags, const char* InText, ...) IM_FMTARGS(2);
	
#pragma endregion
	
#pragma region Layout
	
	/// @brief Draws a ImGui::Dummy to center the next given ItemWidth text in the region available in the x-axis.
	TBIMGUI_API void CenterItem(int32 ItemWidth);
	
	/// @brief Sets the next item width to use the content region available on the x-axis.
	TBIMGUI_API void SetNextItemSpan();
	
	/// @brief Align the next drawn text item with tree node bullet points.
	TBIMGUI_API void AlignTextToTreeNode();
	
	/// @brief Create a Dummy at the same size as a tree node with no text.
	TBIMGUI_API void DummyTreeNode();
	
	/// @brief Get the current tree node depth.
	TBIMGUI_API int32 GetTreeNodeDepth();

	/// @brief Returns the indentation count from the current state.
	TBIMGUI_API int32 GetIndentationCount();
	
#pragma endregion
	
#pragma region Interaction
	
	/// @brief Draws ImGui::Button with checkbox functionality.
	TBIMGUI_API bool CheckboxButton(const char* InText, bool& bCheckedState, TOptional<FColor> InCheckedColor = {});

	/// @brief Draws ImGui::Selectable with checkbox functionality.
    TBIMGUI_API bool CheckboxSelectable(const char* InText, bool& bCheckedState, ImGuiSelectableFlags InFlags = 0);

	/// @brief Draws ImGui::Button text only unless hovered.
	TBIMGUI_API bool TextButton(const char* InText);

	/// @brief Draws ImGui::ColorEdit4 but supports a FColor.
	TBIMGUI_API bool FColorEdit(const char* InLabel, FColor& OutColor, ImGuiColorEditFlags InFlags = 0);

	/// @brief Draws ImGui::ColorEdit4 but supports a FLinearColor.
	TBIMGUI_API bool FLinearColorEdit(const char* InLabel, FLinearColor& OutColor, ImGuiColorEditFlags InFlags = 0);

	/// @brief Draws a drag scalar that accepts doubles.
	TBIMGUI_API bool DragDouble(const char* InLabel, double* OutValue, float InSpeed = 1.0f, double InMin = 0.0, double InMax = 0.0, const char* InFormat = "%.3f", ImGuiSliderFlags flags = 0);
	
	/// @brief Draws a tooltip if the last item was hovered.
	TBIMGUI_API void HoveredTooltip(const char* InText, ...) IM_FMTARGS(1);

	/// @brief Draws a tooltip if the last item was hovered.
	TBIMGUI_API void HoveredTooltipEx(ImGuiHoveredFlags Flags, const char* InText, ...) IM_FMTARGS(2);
	
	/// @brief Filters out invalid file system characters for InputText widget types.
	TBIMGUI_API int InputTextFilter_UnrealFileSystemTypes(ImGuiInputTextCallbackData* InData);

#pragma endregion

#pragma region Table
	
	/// @brief QOL function for going to the start of the next row in a table.
	TBIMGUI_API bool TableNextRowFirstColumn();

	/// @brief Returns true if the column index is enabled, false if not.
	TBIMGUI_API bool TableGetColumnEnabled(int32 ColumnIndex);

	/// @brief Set the table BG color based off a given offset and number of colors.
	TBIMGUI_API void TableSetBgColorByOffset(int32 Offset, int32 NumColors = 6, float ColorBlend = 1.0f);

	/// @brief Set the table BG color based off the current tree node depth and number of colors.
	TBIMGUI_API void TableSetBgColorByDepth(int32 NumColors = 6, float ColorBlend = 1.0f);

	/// @brief Set the active table rows background color if hovered by the mouse.
	TBIMGUI_API void TableRowHighlightOnHovered();

	/// @brief Popup context for table hovered table rows.
	TBIMGUI_API bool TableRowBeginPopupContextItem(const char* InId);
	
#pragma endregion 
	
	/// @brief Format a temp string buffer for inline label assignment.
	TBIMGUI_API const char* FormatTemp(const char* InText, ...) IM_FMTARGS(1);
	
	/// @brief Returns the screenspace absolute position from the main ImGui widget assigned to the main viewport.
	TBIMGUI_API FVector2f GetScreenSpaceAbsolutePosition();
	
	/// @brief Returns the viewports window if it is in fact a window, otherwise returns nullptr.
	TBIMGUI_API ImGuiWindow* GetWindow(ImGuiViewport* InViewport);
	
	/// @brief Returns the viewports window flags if it is in fact a window, otherwise returns empty flags.
	TBIMGUI_API ImGuiWindowFlags GetWindowFlags(ImGuiViewport* InViewport);
	
	/// @brief Add ImGuiWindowFlags to the next window that is created.
	TBIMGUI_API void AddNextWindowFlags(ImGuiWindowFlags InFlagsToAdd);
	
	/// @brief Remove ImGuiWindowFlags from the next window that is created.
	TBIMGUI_API void RemoveNextWindowFlags(ImGuiWindowFlags InFlagsToRemove);
	
	/// @brief Set the given window to always render on-top of other ImGui windows.
	TBIMGUI_API void SetWindowTopmost(const ImGuiWindow* InWindow, bool bTopMost);
	
	/// @brief Return true if the current active ImGui window is the foreground window. False otherwise.
	TBIMGUI_API bool IsForegroundWindow(const ImGuiWindow* InWindow);

	/// @brief Returns true if the combo box with the given ID is open, false otherwise.
	/// @note This can be used before and after the combo is drawn.
	TBIMGUI_API bool IsComboOpen(const char* ComboID);

	/// @brief Return the Unreal ImGui config directory.
	TBIMGUI_API FString ConfigDir();

	/// @brief Clear all window settings.
	TBIMGUI_API void ClearAllWindowSettings();
	
	/// @brief Save the current ImGui ini Settings to the default ConfigDir for the current context.
	TBIMGUI_API void SaveIniSettings();

	/// @brief Load the current ImGui ini Settings to the default ConfigDir for the current context.
	TBIMGUI_API void LoadIniSettings();
	
	/// @brief Flush any internally tracked input states for ImGui IO.
	TBIMGUI_API void FlushInput();
	
	/// @brief Iterates over each ImGui viewport and completes the given function.
	TBIMGUI_API void ForEachViewport(const TFunctionRef<bool(ImGuiViewportP&)>& InFunction);
	
	/// @brief Iterates over each ImGui window and completes the given function.
	TBIMGUI_API void ForEachWindow(const TFunctionRef<bool(ImGuiWindow&)>& InFunction);
	
	/// @brief Copies ImGui array to UE array.
	template<typename SrcType, typename DstType UE_REQUIRES(std::is_constructible_v<DstType, SrcType>)>
	void CopyArray(const ImVector<SrcType>& SrcArray, TArray<DstType>& DstArray)
	{
		DstArray = TArrayView<SrcType>(SrcArray.Data, SrcArray.Size);
	}
}