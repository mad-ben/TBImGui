// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiModulesIntegration.h"

#include "Framework/Docking/TabManager.h"
#include "Slate/SImGuiModules.h"
#include "TBImGuiEditorSettings.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace TBIMGUI_MODULES
{
	const FName ImGuiModulesTabName(TEXT("TBImGuiModules"));
	static bool bRegistered = false;

	static TSharedRef<SDockTab> SpawnImGuiModules(const FSpawnTabArgs&)
	{
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SImGuiModules)];
	}
}

void TBImGuiModulesIntegration::Register()
{
	if (TBIMGUI_MODULES::bRegistered || !UTBImGuiEditorSettings::IsImGuiUIEnabled())
	{
		return;
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TBIMGUI_MODULES::ImGuiModulesTabName, FOnSpawnTab::CreateStatic(&TBIMGUI_MODULES::SpawnImGuiModules))
		.SetDisplayName(NSLOCTEXT("TBImGuiEditor", "ImGuiModulesTab", "ImGui Modules"))
		.SetTooltipText(NSLOCTEXT("TBImGuiEditor", "ImGuiModulesTooltip", "Browse and manage Unreal Engine modules with ImGui."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory());
	TBIMGUI_MODULES::bRegistered = true;
}

void TBImGuiModulesIntegration::Unregister()
{
	if (TBIMGUI_MODULES::bRegistered)
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TBIMGUI_MODULES::ImGuiModulesTabName);
		TBIMGUI_MODULES::bRegistered = false;
	}
}
