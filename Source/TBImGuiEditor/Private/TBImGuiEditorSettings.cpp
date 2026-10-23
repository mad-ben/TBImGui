// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiEditorSettings.h"

#include "TBImGuiEditor.h"
#include "UObject/UnrealType.h"

void UTBImGuiEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UTBImGuiEditorSettings, EditorTheme) || PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UTBImGuiEditorSettings, EditorFontSize))
	{
		FTBImGuiEditorModule::ApplyConfiguredTheme();
	}
}
