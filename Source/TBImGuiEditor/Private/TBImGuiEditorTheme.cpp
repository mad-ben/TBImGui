// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiEditorTheme.h"

#include <Slate/SImGuiWidget.h>

#include "TBImGuiEditorSettings.h"
#include "TBImGuiTypes.h"

namespace TBIMGUI_EDITOR_THEME
{
TArray<TWeakPtr<FTBImGuiContext>> EDITOR_CONTEXTS;

static void ApplyPureDark()
{
	ImGui::StyleColorsDark();

	ImGuiStyle& Style = ImGui::GetStyle();

	ImVec4* Colors = Style.Colors;
	Colors[ImGuiCol_Text] = ImVec4(0.91f, 0.93f, 0.96f, 1.00f);
	Colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.51f, 0.56f, 1.00f);
	Colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.061f, 0.075f, 1.00f);
	Colors[ImGuiCol_ChildBg] = ImVec4(0.070f, 0.077f, 0.092f, 1.00f);
	Colors[ImGuiCol_PopupBg] = ImVec4(0.075f, 0.082f, 0.098f, 0.98f);
	Colors[ImGuiCol_Border] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
	Colors[ImGuiCol_FrameBg] = ImVec4(0.105f, 0.116f, 0.138f, 1.00f);
	Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.18f, 0.23f, 1.00f);
	Colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.23f, 0.31f, 1.00f);
	Colors[ImGuiCol_TitleBg] = ImVec4(0.045f, 0.050f, 0.062f, 1.00f);
	Colors[ImGuiCol_TitleBgActive] = ImVec4(0.075f, 0.086f, 0.110f, 1.00f);
	Colors[ImGuiCol_MenuBarBg] = ImVec4(0.070f, 0.077f, 0.092f, 1.00f);
	Colors[ImGuiCol_Header] = ImVec4(0.14f, 0.29f, 0.47f, 1.00f);
	Colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.39f, 0.65f, 1.00f);
	Colors[ImGuiCol_HeaderActive] = ImVec4(0.14f, 0.33f, 0.56f, 1.00f);
	Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.25f, 0.41f, 1.00f);
	Colors[ImGuiCol_ButtonHovered] = ImVec4(0.16f, 0.38f, 0.64f, 1.00f);
	Colors[ImGuiCol_ButtonActive] = ImVec4(0.13f, 0.31f, 0.54f, 1.00f);
	Colors[ImGuiCol_CheckMark] = ImVec4(0.31f, 0.66f, 1.00f, 1.00f);
	Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.56f, 0.90f, 1.00f);
	Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.34f, 0.69f, 1.00f, 1.00f);
	Colors[ImGuiCol_Tab] = ImVec4(0.09f, 0.12f, 0.17f, 1.00f);
	Colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.38f, 0.64f, 1.00f);
	Colors[ImGuiCol_TabSelected] = ImVec4(0.12f, 0.27f, 0.46f, 1.00f);
	Colors[ImGuiCol_DockingPreview] = ImVec4(0.24f, 0.56f, 0.90f, 0.70f);
	Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.47f, 0.78f, 1.00f);
	Colors[ImGuiCol_SeparatorActive] = ImVec4(0.28f, 0.62f, 1.00f, 1.00f);
}
} // namespace TBIMGUI_EDITOR_THEME

void TBImGuiEditorTheme::ApplyConfiguredToContext(const TSharedPtr<FTBImGuiContext>& Context)
{
	if (!Context.IsValid())
	{
		return;
	}

	FTBImGuiContextScope ContextScope(Context);
	if (ContextScope.IsValid())
	{
		ApplyConfiguredToCurrentContext();
	}
}

void TBImGuiEditorTheme::ApplyConfiguredToCurrentContext()
{
	if (!ImGui::GetCurrentContext())
	{
		return;
	}

	const UTBImGuiEditorSettings* const Settings = UTBImGuiEditorSettings::Get();
	if (!IsValid(Settings))
	{
		return;
	}
	if (const TSharedPtr<FTBImGuiContext> Context = FTBImGuiContext::Get(ImGui::GetCurrentContext()))
	{
		TBIMGUI_EDITOR_THEME::EDITOR_CONTEXTS.RemoveAll([](const TWeakPtr<FTBImGuiContext>& WeakContext)
		                                                { return !WeakContext.IsValid(); });
		if (!TBIMGUI_EDITOR_THEME::EDITOR_CONTEXTS.ContainsByPredicate([&Context](const TWeakPtr<FTBImGuiContext>& WeakContext)
		                                                               { return WeakContext.Pin() == Context; }))
		{
			TBIMGUI_EDITOR_THEME::EDITOR_CONTEXTS.Add(Context);
		}
	}
	const float FontScaleDpi = ImGui::GetStyle().FontScaleDpi;
	ImGui::GetStyle() = ImGuiStyle();
	ImGui::GetStyle().FontScaleDpi = FontScaleDpi;
	ImGui::GetStyle().FontSizeBase = FMath::Clamp(Settings->EditorFontSize, 8.0f, 32.0f);

	switch (Settings->EditorTheme)
	{
	case ETBImGuiEditorTheme::Dark:
		ImGui::StyleColorsDark();
		break;
	case ETBImGuiEditorTheme::Light:
		ImGui::StyleColorsLight();
		break;
	case ETBImGuiEditorTheme::Classic:
		ImGui::StyleColorsClassic();
		break;
	case ETBImGuiEditorTheme::PureDark:
	default:
		TBIMGUI_EDITOR_THEME::ApplyPureDark();
		break;
	}
	ImGuiStyle& Style = ImGui::GetStyle();
	Style.WindowRounding = 0.0f;
	Style.ChildRounding = 0.0f;
	Style.PopupRounding = 0.0f;
	Style.FrameRounding = 0.0f;
	Style.ScrollbarRounding = 0.0f;
	Style.GrabRounding = 0.0f;
	Style.ImageRounding = 0.0f;
	Style.TabRounding = 0.0f;
	Style.TreeLinesRounding = 0.0f;
	Style.DragDropTargetRounding = 0.0f;
}

void TBImGuiEditorTheme::ApplyConfiguredToAllContexts()
{
	const TArray<TWeakPtr<FTBImGuiContext>> Contexts = TBIMGUI_EDITOR_THEME::EDITOR_CONTEXTS;
	for (const TWeakPtr<FTBImGuiContext>& WeakContext : Contexts)
	{
		ApplyConfiguredToContext(WeakContext.Pin());
	}
}
