// Copyright Lewis Scrivens. All Rights Reserved.
#include "TBImGuiOutlinerIntegration.h"

#include "SceneOutlinerModule.h"
#include "SSceneOutliner.h"
#include "Slate/SImGuiSceneOutliner.h"
#include "TBImGuiEditorSettings.h"

namespace TBIMGUI_OUTLINER
{
const FName OUTLINER_FACTORY_OWNER(TEXT("TBImGuiEditor"));

static TSharedPtr<SWidget> MakeOutlinerTree(TSharedRef<SSceneOutliner> Outliner)
{
	// Actor pickers, component pickers, and asset-specific outliners keep their native views.
	const FName Identifier = Outliner->GetOutlinerIdentifier();
	if (Identifier != TEXT("LevelEditorSceneOutliner") && Identifier != TEXT("LevelEditorSceneOutliner2") && Identifier != TEXT("LevelEditorSceneOutliner3") && Identifier != TEXT("LevelEditorSceneOutliner4"))
	{
		return nullptr;
	}
	return SNew(SImGuiSceneOutliner).Outliner(Outliner);
}
} // namespace TBIMGUI_OUTLINER

void TBIMGUI_OUTLINER_INTEGRATION::Register()
{
	if (UTBImGuiEditorSettings::IsImGuiUIEnabled())
	{
		FModuleManager::LoadModuleChecked<FSceneOutlinerModule>(TEXT("SceneOutliner"))
		    .RegisterTreeContentFactory(TBIMGUI_OUTLINER::OUTLINER_FACTORY_OWNER, FSceneOutlinerTreeContentFactory::CreateStatic(&TBIMGUI_OUTLINER::MakeOutlinerTree));
	}
}

void TBIMGUI_OUTLINER_INTEGRATION::Unregister()
{
	if (FSceneOutlinerModule* Module = FModuleManager::GetModulePtr<FSceneOutlinerModule>(TEXT("SceneOutliner")))
	{
		Module->UnregisterTreeContentFactory(TBIMGUI_OUTLINER::OUTLINER_FACTORY_OWNER);
	}
}
