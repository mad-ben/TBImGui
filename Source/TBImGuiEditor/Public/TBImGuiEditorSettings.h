// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>

#include "TBImGuiEditorSettings.generated.h"

UENUM(BlueprintType)
enum class ETBImGuiEditorTheme : uint8
{
	PureDark UMETA(DisplayName = "Pure ImGui Dark"),
	Dark UMETA(DisplayName = "ImGui Dark"),
	Light UMETA(DisplayName = "ImGui Light"),
	Classic UMETA(DisplayName = "ImGui Classic")
};

/// @brief TBImGuiEditor settings to control the flow of ImGui within the editor.
UCLASS(Config = "Editor", DefaultConfig, meta = (DisplayName = "ImGui Toolbox Editor"))
class TBIMGUIEDITOR_API UTBImGuiEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/// @brief Return the singleton settings object.
	static const UTBImGuiEditorSettings* Get()
	{
		return GetDefault<UTBImGuiEditorSettings>();
	}

	/// Begin UDeveloperSettings Override
	virtual FName GetCategoryName() const override
	{
		return TEXT("Toolbox");
	}
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	/// End UDeveloperSettings Override

	/// Should an ImGui context be created for the editor?
	/// @note Requires editor restart to take effect.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bCreateEditorContext = false;

	/// Should ImGui render a demo window when the editor opens?
	/// This is intended for testing the plugin is working when first adding to the project.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bShowDemoWindow = false;

	/// Visual theme used by the editor ImGui context. This does not alter Slate/Starship widgets.
	/// Changes are applied immediately while the editor context is active.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	ETBImGuiEditorTheme EditorTheme = ETBImGuiEditorTheme::Dark;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (DisplayName = "Editor Font Size (px)", ClampMin = "8", ClampMax = "32", UIMin = "8", UIMax = "32", ToolTip = "Font size for ImGui editor panels. Changes apply to open panels."))
	float EditorFontSize = 14.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (DisplayName = "Content Browser Thumbnail Size (px)", ClampMin = "32", ClampMax = "256", UIMin = "32", UIMax = "256", ToolTip = "Shared thumbnail size for ImGui Content Browsers and the Content Drawer. Also updated by their thumbnail sliders."))
	float ContentBrowserThumbnailSize = 112.0f;

	/// Enable all ImGui editor UI integrations. Requires an editor restart.
	/// The config property name is retained to preserve the existing Output Log preference.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (DisplayName = "Enable ImGui UI", ConfigRestartRequired = true))
	bool bPreferImGuiOutputLog = true;

	/// Shared gate for every editor UI port; independent of the optional demo context.
	static bool IsImGuiUIEnabled()
	{
		const UTBImGuiEditorSettings* Settings = Get();
		return IsValid(Settings) && Settings->bPreferImGuiOutputLog;
	}
};
