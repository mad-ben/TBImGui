// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiDemo.h"

#include <Engine/Texture2D.h>
#include <Math/UnrealMathUtility.h>
#include <UObject/StrongObjectPtr.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
#include <imgui_internal.h>
#if WITH_IMPLOT
#include <implot.h>
#endif
#if WITH_IMPLOT3D
#include <implot3d.h>
#endif
#if WITH_NETIMGUI
#include <NetImgui_Api.h>
#endif
THIRD_PARTY_INCLUDES_END

#include "TBImGuiUtils.h"
#include "TBImGuiContext.h"
#include "Fonts/IconsFontAwesome.h"

namespace
{
	void ShowInfoExamples()
	{
		ImGui::SeparatorText("ABOUT");
		{
			IMGUI_INDENT_SCOPE();
			
			if (ImGui::BeginTable("##AboutTable", 2, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Version");
				ImGui::TableNextColumn();
				ImGui::Text("%s", TCHAR_TO_ANSI(*FTBImGuiUtils::GetPluginVersionName()));
				
#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_SLATE
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Render Mode");
				ImGui::TableNextColumn();
				ImGui::Text("Slate");
				ImGui::HoveredTooltip("Use Slate to render ImGui content by batching custom vertices.");
#elif IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Render Mode");
				ImGui::TableNextColumn();
				ImGui::Text("Native");
				ImGui::HoveredTooltip("Translate ImGui draw commands directly into RHI draw calls (inside a Slate custom element).");
#endif
				
				ImGui::EndTable();
			}
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::SeparatorText("CONTEXT");
		TSharedPtr<FTBImGuiContext> ActiveContext = FTBImGuiContext::Get(ImGui::GetCurrentContext());
		if (ActiveContext.IsValid())
		{
			IMGUI_INDENT_SCOPE();
			
			const FTBImGuiContextParams& Params = ActiveContext->GetContextParams();
			
			if (ImGui::BeginTable("##ContextTable", 2, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Context Name");
				ImGui::TableNextColumn();
				ImGui::Text("%s", TCHAR_TO_ANSI(*Params.ContextName));
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Viewport Mode");
				ImGui::TableNextColumn();
				ImGui::Text("%s", TCHAR_TO_ANSI(*UEnum::GetValueOrBitfieldAsString(Params.ViewportMode)));
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Docking Support");
				ImGui::TableNextColumn();
				ImGui::Text(Params.bDockingSupport ? "true" : "false");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Headless");
				ImGui::TableNextColumn();
				ImGui::Text(Params.bHeadless ? "true" : "false");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Windows Always On Top  ");
				ImGui::TableNextColumn();
				ImGui::Text(Params.bWindowsAlwaysOnTop ? "true" : "false");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Raw User Data");
				ImGui::TableNextColumn();
				ImGui::Text("%p", Params.RawUserData);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Object User Data");
				ImGui::TableNextColumn();
				ImGui::Text("%s", TCHAR_TO_ANSI(*GetNameSafe(Params.ObjectUserDataWeak.Get())));
				
				ImGui::TableNextRowFirstColumn();
				ImGui::Text("Input Mode");
				ImGui::TableNextColumn();
				ImGui::Text("%s", TCHAR_TO_ANSI(*UEnum::GetValueOrBitfieldAsString(ActiveContext->GetInputMode())));
				
				ImGui::EndTable();
			}
		}
	}
	
	void ShowTextureExamples()
	{
		auto SendTextureButton = [] (UTexture2D* InTexture)
		{
#if WITH_NETIMGUI
			IMGUI_ID_SCOPE(InTexture);
			
			const bool bTextureValid = IsValid(InTexture);
			const bool bNetImGuiConnected = NetImgui::IsDrawingRemote();
			IMGUI_DISABLE_CONDITION_SCOPE(!bTextureValid || !bNetImGuiConnected);
			if (ImGui::Button("Send to NetImGui"))
			{
				FTBImGuiUtils::SendTextureToNetImGui(InTexture);
			}
			
			if (!bNetImGuiConnected)
			{
				ImGui::HoveredTooltip("NetImGui is not connected.");
			}
			else if (!bTextureValid)
			{
				ImGui::HoveredTooltip("Invalid texture cannot be sent to NetImGui.");
			}
			else
			{
				ImGui::HoveredTooltip("Send the texture through to NetImGui connection.");
			}
#endif
		};
		
		ImGui::SeparatorText("CALIBRATION TEXTURE");
		{
			IMGUI_INDENT_SCOPE();
			
			const FString CalibrationTexturePath = TEXT("/Engine/EngineMaterials/DefaultCalibrationGrayscale.DefaultCalibrationGrayscale");
			ImGui::TextSelectable(TCHAR_TO_UTF8(*CalibrationTexturePath));
			UTexture2D* const CalibrationTexture = LoadObject<UTexture2D>(nullptr, CalibrationTexturePath);
			ImGui::Image(ImTextureRef(CalibrationTexture), ImVec2(250.0f, 250.0f));
			SendTextureButton(CalibrationTexture);
			ImGui::Spacing();
		}
		
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::SeparatorText("MISSING TEXTURE");
		{
			IMGUI_INDENT_SCOPE();
			
#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_NONE
			ImGui::Text("Mode: None");
			ImGui::HoveredTooltip("Don't render anything for missing textures. Skip the draw command entirely.");
#elif IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_EMPTY
			ImGui::Text("Mode: Empty");
			ImGui::HoveredTooltip("Use an empty texture resource for missing textures. Let the renderer decide. This is usually solid white.");
#elif IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_CUSTOM_COLOR
			ImGui::Text("Mode: Custom Color");
			ImGui::HoveredTooltip("Use a custom solid color texture for missing textures.");
			ImGui::Text("Custom Color ");
			ImGui::SameLine();
			FColor CustomColor = CustomMissingTextureColor;
			ImGui::FColorEdit("##CustomColorTexture", CustomColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
#elif IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_ENGINE_DEFAULT
			ImGui::Text("Mode: Engine Default");
			ImGui::HoveredTooltip("Use Unreal Engines default texture for missing textures.");
#endif
			
			TSharedPtr<FTBImGuiContext> ActiveContext = FTBImGuiContext::Get(ImGui::GetCurrentContext());
			if (ActiveContext.IsValid())
			{
				UTexture2D* const MissingTexture = ActiveContext->GetMissingTexture();
				ImGui::Image(ImTextureRef(MissingTexture), ImVec2(250.0f, 250.0f));
				SendTextureButton(MissingTexture);
			}
		}
	}
	
	void ShowFontExamples()
	{
		ImGuiIO& IO = ImGui::GetIO();
		ImFont* const ActiveFont = ImGui::GetFont();
		if (!ActiveFont || !IO.Fonts || IO.Fonts->Fonts.Size == 0)
		{
			ImGui::Text("No fonts are available...");
			return;
		}
		
		ImGui::SeparatorText("FONTS");
		{
			IMGUI_INDENT_SCOPE();
			
			static int32 SelectedFontIndex = 0;
			static float PreviewSize = 24.0f;
			SelectedFontIndex = FMath::Clamp(SelectedFontIndex, 0, IO.Fonts->Fonts.Size - 1);
		
			ImFont* SelectedFont = IO.Fonts->Fonts[SelectedFontIndex];
			if (ImGui::BeginCombo("Font", SelectedFont->GetDebugName()))
			{
				for (int32 FontIndex = 0; FontIndex < IO.Fonts->Fonts.Size; ++FontIndex)
				{
					ImFont* const Font = IO.Fonts->Fonts[FontIndex];
					const bool bSelected = SelectedFontIndex == FontIndex;
					ImGui::PushID(FontIndex);
					if (ImGui::Selectable(Font->GetDebugName(), bSelected))
					{
						SelectedFontIndex = FontIndex;
						SelectedFont = Font;
					}
				
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				
					ImGui::PopID();
				}
			
				ImGui::EndCombo();
			}
		
			ImGui::SliderFloat("Preview size", &PreviewSize, 5.0f, 100.0f, "%.0f px");
			ImGui::Spacing();
		
			ImGui::PushFont(SelectedFont, PreviewSize);
			{
				ImGui::Text("Preview font: %s", SelectedFont->GetDebugName());
				ImGui::Spacing();
				ImGui::Text("The quick brown fox jumps over the lazy dog");
				ImGui::Spacing();
				ImGui::Text(ICON_FA_EXPLOSION " Font Awesome 7 Free is merged onto the end of the font atlas. " ICON_FA_EXPLOSION);
			}
			ImGui::PopFont();
			ImGui::Spacing();
		}
		
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::SeparatorText("FONT AWESOME 7 ICONS");
  		{
			IMGUI_INDENT_SCOPE();
			
			ImGui::Text("For unicode mappings see IconsFontAwesome.h");
			
			static char Filter[64] = "";
			static float FontSize = 24.0f;
			
			ImGui::InputText("Filter (hex)", Filter, IM_COUNTOF(Filter));
			ImGui::SliderFloat("Font size", &FontSize, 5.0f, 100.0f, "%.0f px");
		
      		const FString FilterStr = ANSI_TO_TCHAR(Filter);
      		const bool bHasFilter = !FilterStr.IsEmpty();
      		auto PassesFilter = [&](ImWchar Char) -> bool
      		{
      		    if (!bHasFilter)
      		    {
      		    	return true;
      		    }
      			
      		    const FString HexString = FString::Printf(TEXT("%x"), Char);
      		    return HexString.Contains(FilterStr, ESearchCase::IgnoreCase);
      		};
		
      		// Show how many icons there are.
      		int32 IconCount = 0;
      		for (ImWchar Cp = ICON_MIN_FA; Cp <= ICON_MAX_FA; ++Cp)
      		{
      		    if (ActiveFont->IsGlyphInFont(Cp) && PassesFilter(Cp))
      		    {
      		    	++IconCount;
      		    }
      		}
      		ImGui::Text("Icons: %d", IconCount);
			
      		// Cell scales with font size + padding.
			ImGuiStyle& Style = ImGui::GetStyle();
			const float CellPadding = FMath::Max(Style.FramePadding.x, Style.ItemSpacing.y);
      		const float CellSize = (FontSize * Style.FontScaleDpi) + CellPadding * 4.0f;
			const float AvailableSpace = ImGui::GetContentRegionAvail().x;
      		const float Spacing = ImGui::GetStyle().ItemSpacing.x;
      		const int32 Columns = FMath::Max(1, FMath::Floor(AvailableSpace / (CellSize + Spacing)));
			
      		ImGui::BeginChild("##IconsGrid", ImVec2(0.0f, 400.0f), true);
			
      		int32 ColumnIndex = 0;
      		for (ImWchar Char = ICON_MIN_FA; Char <= ICON_MAX_FA; ++Char)
      		{
      		    if (!ActiveFont->IsGlyphInFont(Char) || !PassesFilter(Char))
      		    {
      		    	continue;
      		    }
		
      		    char Utf8[5] = {};
      		    ImTextCharToUtf8(Utf8, Char);
      			
      		    {
      		    	IMGUI_ID_SCOPE(Char);
      		    	
      		    	ImGui::PushFont(nullptr, FontSize);
	      		    {
      		    		if (ImGui::Button(Utf8, ImVec2(CellSize, CellSize)))
      		    		{
      		    			char ClipBuf[32];
      		    			ImFormatString(ClipBuf, IM_COUNTOF(ClipBuf), "U+%04X", Char);
      		    			ImGui::SetClipboardText(ClipBuf);
      		    		}
	      		    }
      		    	ImGui::PopFont();
      			
      		    	if (ImGui::IsItemHovered())
      		    	{
      		    		ImGui::SetTooltip("U+%04X  (click to copy)", Char);
      		    	}
      		    }
      			
      		    ++ColumnIndex;
      		    if (ColumnIndex < Columns)
      		    {
      		    	ImGui::SameLine();
      		    }
      		    else
      		    {
      		    	ColumnIndex = 0;
      		    }
      		}
			
      		ImGui::EndChild();
  		}
		
		ImGui::Spacing();
	}
	
	constexpr ImGuiTreeNodeFlags DemoTreeNodeFlags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DrawLinesFull;
	
	void TableDemo_BuildRowRecursive(const int32 InMaxDepth, const int32 InCurrentDepth, const int32 InTreeNodeCount)
	{
		if (InCurrentDepth >= InMaxDepth)
		{
			ImGui::TableNextRowFirstColumn();
			ImGui::TableSetBgColorByDepth();
			ImGui::AlignTextToTreeNode();
			ImGui::TextUnformatted("End of the line!");
			return;
		}
		
		for (int32 i = 0; i < InTreeNodeCount; i++)
		{
			ImGui::TableNextRowFirstColumn();
			ImGui::TableSetBgColorByDepth();
			if (ImGui::TreeNodeEx(ImGui::FormatTemp("Tree Node %i : %i", InCurrentDepth, i), DemoTreeNodeFlags))
			{
				TableDemo_BuildRowRecursive(InMaxDepth, InCurrentDepth + 1, InTreeNodeCount);
				ImGui::TreePop();
			}
		}
	}
	
	void ShowImGuiExtraExamples()
	{
		ImGui::SeparatorText("TEXT");
		{
			IMGUI_INDENT_SCOPE();
			
			if (ImGui::BeginTable("##TextTable", 2, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TextSelectable()");
				ImGui::HoveredTooltip("Draws text similar to ImGui::Text that can be selected and copied.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextSelectable("The text can be selected with the mouse and copied!");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TextCopy()");
				ImGui::HoveredTooltip("Draws text with copy button at the end.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextCopy("The text comes with a copy button -> ");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TextAligned()");
				ImGui::HoveredTooltip("Draws text after calling ImGui::AlignTextToFramePadding. \n"
					"Useful for drawing text in horizontal columns to align it with the adjacent widgets vertically.");
				ImGui::TableNextColumn();
				ImGui::TextAligned("All text in this section is aligned to the active frame padding.");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TextCentered()");
				ImGui::HoveredTooltip("Draws text in the center of the available width in the current cell.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextCentered("This text is centered in the current cell!");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TextTooltip()");
				ImGui::HoveredTooltip("Draws the given tooltip under a text icon if the last item was hovered.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextTooltip("This is a tooltip example!");
				
				ImGui::EndTable();
			}
		}
		
		ImGui::SeparatorText("LAYOUT");
		{
			IMGUI_INDENT_SCOPE();
			
			if (ImGui::BeginTable("##LayoutTable", 2, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::CenterItem()");
				ImGui::HoveredTooltip("Draws a ImGui::Dummy to center the next given ItemWidth text in the region available in the x-axis.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				const ImGuiStyle& Style = ImGui::GetStyle();
				const ImVec2 TextSize = ImGui::CalcTextSize("This is a centered button!", nullptr, false);
				ImVec2 ButtonSize = ImGui::CalcItemSize(ImVec2(), TextSize.x + Style.FramePadding.x * 2.0f, TextSize.y + Style.FramePadding.y * 2.0f);
				ImGui::CenterItem(ButtonSize.x);
				ImGui::Button("Centered button!");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::SetNextItemSpan()");
				ImGui::HoveredTooltip("Sets the next item width to use the content region available on the x-axis.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				const char* ComboExampleItems[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF" };
				static int ComboSelectedIndex = 3;
				ImGui::SetNextItemSpan();
				ImGui::Combo("", &ComboSelectedIndex, ComboExampleItems, IM_COUNTOF(ComboExampleItems));
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::GetIndentationCount()");
				ImGui::HoveredTooltip("Returns the indentation count from the current state.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TextAligned("Indentation Count: %i", ImGui::GetIndentationCount());
				
				ImGui::TableNextRowFirstColumn();
				ImGui::AlignTextToFramePadding();
				const bool bAlignTextToTreeNodeOpen = ImGui::TreeNodeEx("ImGui::AlignTextToTreeNode()", DemoTreeNodeFlags);
				ImGui::HoveredTooltip("Align the next drawn text item with tree node bullet points.");
				if (bAlignTextToTreeNodeOpen)
				{
					if (ImGui::TreeNodeEx("Tree Node", DemoTreeNodeFlags))
					{
						ImGui::TextAligned("Empty...");
						ImGui::TreePop();
					}
					ImGui::AlignTextToTreeNode();
					ImGui::Text("Aligned Text!");
					ImGui::Text("Normal Text!");
					ImGui::TreePop();
				}
				
				ImGui::TableNextRowFirstColumn();
				ImGui::AlignTextToFramePadding();
				const bool bDummyTreeNodeOpen = ImGui::TreeNodeEx("ImGui::DummyTreeNode()", DemoTreeNodeFlags);
				ImGui::HoveredTooltip("Create a Dummy at the same size as a tree node with no text.");
				if (bDummyTreeNodeOpen)
				{
					if (ImGui::TreeNodeEx("Tree Node A", DemoTreeNodeFlags))
					{
						ImGui::TextAligned("Empty...");
						ImGui::TreePop();
					}
					ImGui::DummyTreeNode();
					if (ImGui::TreeNodeEx("Tree Node B", DemoTreeNodeFlags))
					{
						ImGui::TextAligned("Empty...");
						ImGui::TreePop();
					}
					if (ImGui::TreeNodeEx("Tree Node C", DemoTreeNodeFlags))
					{
						ImGui::TextAligned("Empty...");
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				
				ImGui::TableNextRowFirstColumn();
				ImGui::AlignTextToFramePadding();
				const bool bTreeNodeDepthOpen = ImGui::TreeNodeEx("ImGui::GetTreeNodeDepth()", DemoTreeNodeFlags);
				ImGui::HoveredTooltip("Get the current tree node depth.");
				if (bTreeNodeDepthOpen)
				{
					if (ImGui::TreeNodeEx("Tree Node A", DemoTreeNodeFlags))
					{
						if (ImGui::TreeNodeEx("Tree Node B", DemoTreeNodeFlags))
						{
							ImGui::AlignTextToTreeNode();
							ImGui::Text("Tree Depth: %i", ImGui::GetTreeNodeDepth());
							ImGui::TreePop();
						}
						
						ImGui::AlignTextToTreeNode();
						ImGui::Text("Tree Depth: %i", ImGui::GetTreeNodeDepth());
						ImGui::TreePop();
					}
					
					ImGui::AlignTextToTreeNode();
					ImGui::Text("Tree Depth: %i", ImGui::GetTreeNodeDepth());
					ImGui::TreePop();
				}
				
				ImGui::EndTable();
			}
		}
		
		ImGui::SeparatorText("INTERACTION");
		{
			IMGUI_INDENT_SCOPE();
			
			if (ImGui::BeginTable("##InteractionTable", 2, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::CheckboxButton()");
				ImGui::HoveredTooltip("Draws ImGui::Button with checkbox functionality.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				static bool bExampleCheckboxButtonState = false;
				ImGui::CheckboxButton(ICON_FA_LIGHTBULB, bExampleCheckboxButtonState);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::CheckboxSelectable()");
				ImGui::HoveredTooltip("Draws ImGui::Selectable with checkbox functionality.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				static bool bExampleCheckboxSelectableState = true;
				ImGui::CheckboxSelectable("Selectable", bExampleCheckboxSelectableState);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TextButton()");
				ImGui::HoveredTooltip("Draws ImGui::Button text only unless hovered.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				static int32 ExampleCounter = 0;
				if (ImGui::TextButton("Interactable text!"))
				{
					ExampleCounter++;
				}
				ImGui::SameLine();
				ImGui::TextAligned("%i", ExampleCounter);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::FColorEdit()");
				ImGui::HoveredTooltip("Draws ImGui::ColorEdit4 but supports a FColor values directly.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::SetNextItemSpan();
				static FColor ExampleFColor = FColor::Blue;
				ImGui::FColorEdit("##ColorExample", ExampleFColor);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::FLinearColorEdit()");
				ImGui::HoveredTooltip("Draws ImGui::ColorEdit4 but supports a FLinearColor.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::SetNextItemSpan();
				static FLinearColor ExampleFLinearColor = FColor::Green;
				ImGui::FLinearColorEdit("##LinearColorExample", ExampleFLinearColor);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::DragDouble()");
				ImGui::HoveredTooltip("Draws a drag scalar that accepts doubles directly.");
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::SetNextItemSpan();
				static double DragDoubleExample = 20.0;
				ImGui::DragDouble("##DragDoubleExample", &DragDoubleExample, 0.5f, 0.0, 100.0);
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::InputTextFilter_UnrealFileSystemTypes()  ");
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_DelayNormal))
				{
					if (ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_NoFocusOnAppearing))
					{
						ImGui::TextUnformatted("Filters out invalid file system characters for InputText widget types.");
						ImGui::TextUnformatted("Try entering any of the following characters: /?:&\\*\"<>|%#@^");
						ImGui::EndTooltip();
					}
				}
				
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::SetNextItemSpan();
				static char ExampleTextBuffer[256] = "This text will not accept unreal file system types!"; 
				ImGui::InputText("##InputTextExample", ExampleTextBuffer, IM_COUNTOF(ExampleTextBuffer), 
					ImGuiInputTextFlags_CallbackCharFilter, ImGui::InputTextFilter_UnrealFileSystemTypes);
				
				ImGui::EndTable();
			}
		}
		
		ImGui::SeparatorText("TABLE");
		{
			IMGUI_INDENT_SCOPE();
			
			static bool bIsThirdColumnEnabled = false;
			
			if (ImGui::BeginTable("##InteractionTable", bIsThirdColumnEnabled ? 3 : 2, ImGuiTableFlags_PadOuterX | 
				ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				if (bIsThirdColumnEnabled)
				{
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				}
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TableNextRowFirstColumn()");
				ImGui::HoveredTooltip("QOL function for going to the start of the next row in a table.");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TableGetColumnEnabled()");
				ImGui::HoveredTooltip("Returns true if the column index is enabled, false if not.\n"
					"Use the IsThirdColumnEnabled flag to see it in action!");
				ImGui::TableNextColumn();
				ImGui::TextAligned("Column 3 enabled: %s", ImGui::TableGetColumnEnabled(2) ? "true" : "false");
				
				if (bIsThirdColumnEnabled)
				{
					ImGui::TableNextColumn();
					ImGui::TextCentered(ICON_FA_FACE_SMILE);
				}
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TableRowHighlightOnHovered();
				ImGui::TextAligned("ImGui::TableRowHighlightOnHovered()");
				ImGui::HoveredTooltip("Set the active table rows background color if hovered by the mouse.");
				ImGui::TableNextColumn();
				ImGui::TextAligned("This row can be hovered with the mouse!");
				
				ImGui::TableNextRowFirstColumn();
				ImGui::TextAligned("ImGui::TableRowBeginPopupContextItem()");
				ImGui::HoveredTooltip("Popup context for table hovered table rows.");
				ImGui::TableNextColumn();
				ImGui::TextAligned("Right click this row for a context menu!");
				if (ImGui::TableRowBeginPopupContextItem("##RowPopupContextExample"))
				{
					ImGui::Selectable("Option 1");
					ImGui::Selectable("Option 2");
					ImGui::Selectable("Option 3");
					ImGui::EndPopup();
				}
				
				ImGui::TableNextRowFirstColumn();
				ImGui::AlignTextToFramePadding();
				const bool bTableSetBgColorByOffsetOpen = ImGui::TreeNodeEx("ImGui::TableSetBgColorByOffset()", DemoTreeNodeFlags);
				ImGui::HoveredTooltip("Set the table BG color based off a given offset and number of colors.");
				if (bTableSetBgColorByOffsetOpen)
				{
					for (int32 i = 0; i < 10; i++)
					{
						ImGui::TableNextRowFirstColumn();
						ImGui::TableSetBgColorByOffset(i);
						ImGui::TextAligned("Row %i", i);
					}
					
					ImGui::TreePop();
				}
				
				ImGui::TableNextRowFirstColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::TableSetBgColorByDepth();
				const bool bTableSetBgColorByDepthOpen = ImGui::TreeNodeEx("ImGui::TableSetBgColorByDepth()", DemoTreeNodeFlags);
				ImGui::HoveredTooltip("Set the table BG color based off the current tree node depth and number of colors.");
				if (bTableSetBgColorByDepthOpen)
				{
					TableDemo_BuildRowRecursive(6, 0, 2);
					ImGui::TreePop();
				}
				
				ImGui::EndTable();
			}
			
			ImGui::CheckboxSelectable("Enable third column?", bIsThirdColumnEnabled);
		}
	}
	
	void ShowGraphsExamples()
	{
		IMGUI_UNINDENT_SCOPE();
		
#if WITH_IMPLOT
		ImGui::SeparatorText("IMPLOT");
		
		static TTBImGuiRingBuffer<float> SineBufferX = TTBImGuiRingBuffer<float>(1024);
		static TTBImGuiRingBuffer<float> SineBufferY = TTBImGuiRingBuffer<float>(1024);
		static TTBImGuiRingBuffer<float> FrameBufferX = TTBImGuiRingBuffer<float>(1024);
		static TTBImGuiRingBuffer<float> FrameBufferY = TTBImGuiRingBuffer<float>(1024);
		
		static float ElapsedTime = 0.0f;
		const float DeltaSeconds = ImGui::GetIO().DeltaTime;
		constexpr float VisibleTime = 10.0f;
		ElapsedTime += DeltaSeconds;
		
		const FVector2f SineElement = FVector2f(ElapsedTime, FMath::Sin(ElapsedTime * UE_TWO_PI * 0.5f));
		const FVector2f FrameElement = FVector2f(ElapsedTime, DeltaSeconds * 1000.0f);
		
		SineBufferX.AddElement(SineElement.X);
		SineBufferY.AddElement(SineElement.Y);
		FrameBufferX.AddElement(FrameElement.X);
		FrameBufferY.AddElement(FrameElement.Y);
		
		if (ImPlot::BeginPlot("Sine wave", ImVec2(-1.0f, 220.0f)))
		{
			ImPlot::SetupAxes("Time (s)", "##Value", ImPlotAxisFlags_None, ImPlotAxisFlags_NoLabel);
			ImPlot::SetupAxisLimits(ImAxis_X1, ElapsedTime - VisibleTime, ElapsedTime, ImPlotCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -1.2f, 1.2f, ImPlotCond_Once);
			
			ImPlotSpec PlotSpec;
			PlotSpec.LineColor = ImGui::ToImVec4(FColor::Purple);
			PlotSpec.LineWeight = 2.0f;
			PlotSpec.Offset = SineBufferX.GetWriteOffset();
			ImPlot::PlotLine("sin(t)", SineBufferX.GetData(), SineBufferY.GetData(), 
				SineBufferX.Num(), PlotSpec);
			
			ImPlot::EndPlot();
		}
		
		ImGui::Spacing();
		ImGui::Spacing();
		
		if (ImPlot::BeginPlot("Update rate", ImVec2(-1.0f, 220.0f)))
		{
			ImPlot::SetupAxes("##Time", "Frame (ms)", ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_None);
			ImPlot::SetupAxisLimits(ImAxis_X1, ElapsedTime - VisibleTime, ElapsedTime, ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 30.0f, ImPlotCond_Once);
			
			ImPlotSpec PlotSpec;
			PlotSpec.LineColor = ImGui::ToImVec4(FColor::Green);
			PlotSpec.LineWeight = 2.0f;
			PlotSpec.Offset = FrameBufferX.GetWriteOffset();
			ImPlot::PlotLine("Frame", FrameBufferX.GetData(), FrameBufferY.GetData(), 
				FrameBufferX.Num(), PlotSpec);
			
			double BaseLine = 1000.0 / 60.0;
			ImPlot::PlotInfLines("60 FPS", &BaseLine, 1, { ImPlotProp_Flags, ImPlotInfLinesFlags_Horizontal });

			ImPlot::EndPlot();
		}
#endif
		
#if WITH_IMPLOT3D
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::SeparatorText("IMPLOT3D");
		
		static bool bExtractMesh = false;
		static TArray<FVector3f> CachedVertices;
		static TArray<uint32> CachedIndices;
		static FBoxSphereBounds CachedBounds;
		
		if (!bExtractMesh)
		{
			UStaticMesh* const Mesh = LoadObject<UStaticMesh>(nullptr, 
			TEXT("/Engine/EditorMeshes/MatineeCam_SM.MatineeCam_SM"));
			
			if (IsValid(Mesh))
			{
				CachedBounds = Mesh->GetBounds();
				bExtractMesh = FTBImGuiUtils::ExtractMesh(Mesh, CachedVertices, CachedIndices);
			}
		}
		else if (ImPlot3D::BeginPlot("Camera Static Mesh"))
		{
			const FVector Origin = CachedBounds.Origin;
			const double Radius = FMath::Max(CachedBounds.SphereRadius * 1.1, 1.0);
			
			ImPlot3D::SetupAxes("X", "Y", "Z");
			ImPlot3D::SetupAxesLimits(
				Origin.X - Radius, Origin.X + Radius,
				Origin.Y - Radius, Origin.Y + Radius,
				Origin.Z - Radius, Origin.Z + Radius);

			ImPlot3DSpec PlotSpec;
			PlotSpec.Stride = sizeof(FVector3f);
			PlotSpec.Flags = ImPlot3DMeshFlags_NoMarkers;
			PlotSpec.FillColor = ImVec4(0.08f, 0.35f, 0.75f, 0.65f);
			PlotSpec.LineColor = ImVec4(0.35f, 0.70f, 1.00f, 0.90f);
			
			ImPlot3D::PlotMesh("MatineeCam_SM", &CachedVertices[0].X, &CachedVertices[0].Y, &CachedVertices[0].Z,
				CachedIndices.GetData(), CachedVertices.Num(), CachedIndices.Num(), PlotSpec);
			
			ImPlot3D::EndPlot();
		}
#endif
	}
}

void FTBImGuiDemo::ShowWindow(bool* bIsOpen)
{
	if (ImGui::Begin("ImGui Toolbox Demo", bIsOpen))
	{
		ShowContent();
	}
	
	ImGui::End();
}

void FTBImGuiDemo::ShowContent()
{
	if (ImGui::CollapsingHeader(ICON_FA_INFO "     Info", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ShowInfoExamples();
		ImGui::Spacing();
		ImGui::Spacing();
	}
	
	if (ImGui::CollapsingHeader(ICON_FA_IMAGE "   Textures"))
	{
		ShowTextureExamples();
		ImGui::Spacing();
		ImGui::Spacing();
	}
	
	if (ImGui::CollapsingHeader(ICON_FA_FONT "   Fonts and icons"))
	{
		ShowFontExamples();
		ImGui::Spacing();
		ImGui::Spacing();
	}
	
	if (ImGui::CollapsingHeader(ICON_FA_SHAPES "   ImGui Additional widgets"))
	{
		ShowImGuiExtraExamples();
		ImGui::Spacing();
		ImGui::Spacing();
	}
	
#if WITH_IMPLOT || WITH_IMPLOT3D
	if (ImGui::CollapsingHeader(ICON_FA_CHART_LINE "   Graphs"))
	{
		ShowGraphsExamples();
	}
#endif
}