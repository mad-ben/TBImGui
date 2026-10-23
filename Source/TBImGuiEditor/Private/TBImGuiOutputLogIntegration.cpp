// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiOutputLogIntegration.h"

#include "Framework/Docking/TabManager.h"
#include "OutputLogModule.h"
#include "Slate/SImGuiOutputLog.h"
#include "TBImGuiEditorSettings.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace TBIMGUI_OUTPUT_LOG_INTEGRATION
{
	const FName SlateOutputLogFallbackTabName = TEXT("TBSlateOutputLogFallback");
	const FName OutputLogFactoryOwner = TEXT("TBImGuiEditor");
	static bool bRegisteredPreferredFactory = false;

	static TSharedRef<SWidget> MakeImGuiOutputLog(bool)
	{
		return SNew(SImGuiOutputLog);
	}

	static TSharedRef<SDockTab> SpawnSlateOutputLog(const FSpawnTabArgs&)
	{
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			FOutputLogModule::Get().MakeOutputLogWidget(FOutputLogCreationParams())
		];
	}

}

void TBImGuiOutputLogIntegration::Register()
{
	const TSharedRef<FWorkspaceItem> LogMenuGroup = WorkspaceMenu::GetMenuStructure().GetDeveloperToolsLogCategory();
	if (UTBImGuiEditorSettings::IsImGuiUIEnabled())
	{
		FOutputLogModule& OutputLogModule = FModuleManager::LoadModuleChecked<FOutputLogModule>(TEXT("OutputLog"));
		OutputLogModule.RegisterOutputLogWidgetFactory(
			TBIMGUI_OUTPUT_LOG_INTEGRATION::OutputLogFactoryOwner,
			FOutputLogWidgetFactory::CreateStatic(&TBIMGUI_OUTPUT_LOG_INTEGRATION::MakeImGuiOutputLog));

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TBIMGUI_OUTPUT_LOG_INTEGRATION::SlateOutputLogFallbackTabName, FOnSpawnTab::CreateStatic(&TBIMGUI_OUTPUT_LOG_INTEGRATION::SpawnSlateOutputLog))
			.SetDisplayName(NSLOCTEXT("TBImGuiEditor", "SlateOutputLogFallbackTab", "Output Log (Slate Fallback)"))
			.SetTooltipText(NSLOCTEXT("TBImGuiEditor", "SlateOutputLogFallbackTooltip", "Open Unreal's original Slate Output Log."))
			.SetGroup(LogMenuGroup);

		TBIMGUI_OUTPUT_LOG_INTEGRATION::bRegisteredPreferredFactory = true;
	}
}

void TBImGuiOutputLogIntegration::Unregister()
{
	if (TBIMGUI_OUTPUT_LOG_INTEGRATION::bRegisteredPreferredFactory)
	{
		FOutputLogModule::Get().UnregisterOutputLogWidgetFactory(TBIMGUI_OUTPUT_LOG_INTEGRATION::OutputLogFactoryOwner);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TBIMGUI_OUTPUT_LOG_INTEGRATION::SlateOutputLogFallbackTabName);
		TBIMGUI_OUTPUT_LOG_INTEGRATION::bRegisteredPreferredFactory = false;
	}
}
