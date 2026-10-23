// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiDetailsIntegration.h"

#include "PropertyEditorModule.h"
#include "Slate/SImGuiDetailsView.h"
#include "TBImGuiEditorSettings.h"

namespace TBIMGUI_DETAILS
{
const FName FACTORY_OWNER(TEXT("TBImGuiEditor"));

static TSharedPtr<SWidget> MakeTree(TSharedRef<IDetailsView> View, const FDetailsViewTreeContext& Context)
{
	const FName Identifier = View->GetIdentifier();
	if (Identifier != TEXT("LevelEditorSelectionDetails") && Identifier != TEXT("LevelEditorSelectionDetails2") && Identifier != TEXT("LevelEditorSelectionDetails3") && Identifier != TEXT("LevelEditorSelectionDetails4"))
	{
		return nullptr;
	}
	return SNew(SImGuiDetailsView).DetailsView(View).TreeContext(Context);
}
} // namespace TBIMGUI_DETAILS

void TBIMGUI_DETAILS_INTEGRATION::Register()
{
	if (UTBImGuiEditorSettings::IsImGuiUIEnabled())
	{
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"))
		    .RegisterTreeContentFactory(TBIMGUI_DETAILS::FACTORY_OWNER, FDetailsViewTreeContentFactory::CreateStatic(&TBIMGUI_DETAILS::MakeTree));
	}
}

void TBIMGUI_DETAILS_INTEGRATION::Unregister()
{
	if (FPropertyEditorModule* Module = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
	{
		Module->UnregisterTreeContentFactory(TBIMGUI_DETAILS::FACTORY_OWNER);
	}
}
