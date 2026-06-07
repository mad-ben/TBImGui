// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>

#include "TBImGuiEditorSettings.generated.h"

/// @brief TBImGuiEditor settings to control the flow of ImGui within the editor.
UCLASS(Config="Editor", DefaultConfig, meta = (DisplayName = "ImGui Editor Toolbox"))
class TBIMGUIEDITOR_API UTBImGuiEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	
	/// @brief Return the singleton settings object.
	static const UTBImGuiEditorSettings* Get()
	{
		return GetDefault<UTBImGuiEditorSettings>();
	}
	
	/// Should an ImGui context be created for the editor?
	/// @note Requires editor restart to take effect.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bCreateEditorContext = false;
	
	/// Should ImGui render a demo window at runtime? 
	/// This is intended for testing the plugin is working when first adding to the project.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bShowDemoWindow = false;
};