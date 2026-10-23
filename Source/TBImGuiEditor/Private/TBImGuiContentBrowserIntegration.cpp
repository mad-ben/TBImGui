// Copyright Lewis Scrivens. All Rights Reserved.
#include "TBImGuiContentBrowserIntegration.h"

#include "ContentBrowserModule.h"
#include "HAL/IConsoleManager.h"
#include "Slate/SImGuiAssetView.h"
#include "TBImGuiEditorSettings.h"

namespace TBIMGUI_CONTENT_BROWSER
{
const FName CONTENT_BROWSER_FACTORY_OWNER(TEXT("TBImGuiEditor"));
TOptional<int32> PREVIOUS_FLAT_THUMBNAIL_DEFAULT;

static TSharedPtr<IContentBrowserViewExtender> MakeImGuiAssetView()
{
	return SNew(SImGuiAssetView);
}
} // namespace TBIMGUI_CONTENT_BROWSER

void TBIMGUI_CONTENT_BROWSER_INTEGRATION::Register()
{
	if (UTBImGuiEditorSettings::IsImGuiUIEnabled())
	{
		if (IConsoleVariable* FlatDefault = IConsoleManager::Get().FindConsoleVariable(TEXT("Editor.MaterialThumbnail.FlatDefault")))
		{
			if (!TBIMGUI_CONTENT_BROWSER::PREVIOUS_FLAT_THUMBNAIL_DEFAULT.IsSet())
			{
				TBIMGUI_CONTENT_BROWSER::PREVIOUS_FLAT_THUMBNAIL_DEFAULT = FlatDefault->GetInt();
				FlatDefault->Set(1, ECVF_SetByCode);
			}
		}
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"))
		    .RegisterPreferredViewExtender(TBIMGUI_CONTENT_BROWSER::CONTENT_BROWSER_FACTORY_OWNER, FContentBrowserModule::FCreateViewExtender::CreateStatic(&TBIMGUI_CONTENT_BROWSER::MakeImGuiAssetView));
	}
}

void TBIMGUI_CONTENT_BROWSER_INTEGRATION::Unregister()
{
	if (TBIMGUI_CONTENT_BROWSER::PREVIOUS_FLAT_THUMBNAIL_DEFAULT.IsSet())
	{
		if (IConsoleVariable* FlatDefault = IConsoleManager::Get().FindConsoleVariable(TEXT("Editor.MaterialThumbnail.FlatDefault")))
		{
			FlatDefault->Set(TBIMGUI_CONTENT_BROWSER::PREVIOUS_FLAT_THUMBNAIL_DEFAULT.GetValue(), ECVF_SetByCode);
		}
		TBIMGUI_CONTENT_BROWSER::PREVIOUS_FLAT_THUMBNAIL_DEFAULT.Reset();
	}
	if (auto* Module = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
	{
		Module->UnregisterPreferredViewExtender(TBIMGUI_CONTENT_BROWSER::CONTENT_BROWSER_FACTORY_OWNER);
	}
}
