// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>

#include "TBImGuiSettings.generated.h"

UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETBNetImGuiContext : uint8
{
	Client			= 1 << 0,
	Standalone		= 1 << 1,
	ListenServer	= 1 << 2,
	DedicatedServer	= 1 << 3,
	
	All				= Client | Standalone | ListenServer | DedicatedServer UMETA(Hidden)
};
ENUM_CLASS_FLAGS(ETBNetImGuiContext)

/// @brief TBImGui plugin settings to control the flow of ImGui within the project.
UCLASS(Config="Game", DefaultConfig, meta = (DisplayName = "ImGui Toolbox"))
class TBIMGUI_API UTBImGuiSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	
	UTBImGuiSettings();
	
#if WITH_EDITOR
	/// Begin UDeveloperSettings Override
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	/// End UDeveloperSettings Override
#endif
	
	/// @brief Returns the singleton settings object.
	static const UTBImGuiSettings* Get();
	
	/// @brief Should create context.
	bool ShouldCreateContext(const ENetMode InNetMode) const;
	bool ShouldCreateContext(const ETBNetImGuiContext InContext) const;
	
	/// @brief Returns all additional project font paths.
	TArray<FString> GetAdditionalFontPaths() const;
	
protected:
	
	/// @brief Populate the DefaultFont options array.
	UFUNCTION()
	TArray<FString> GetFontOptions() const;
	
	/// @brief Returns default font name.
	FString GetDefaultFontName() const;
	
public:
	
	/// Should an ImGui context be created for the given runtime application types?
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "General", meta = (Bitmask, BitmaskEnum = "/Script/TBImGui.ETBNetImGuiContext"))
	uint8 CreateContext = static_cast<uint8>(ETBNetImGuiContext::All);
	
	/// Should ImGui render a demo window at runtime? 
	/// This is intended for testing the plugin is working when first adding to the project.
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	bool bShowDemoWindow = true;
	
	/// ImGui engine frame binding for rendering ImGui content at runtime.
	/// @note If you wish to handle BeginFrame and EndFrame manually, set this is None/nullptr.
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "General", meta = (ShowDisplayNames, MetaStruct = "/Script/TBImGui.TBImGuiFrameBinding_Base", ExcludeBaseStruct))
	TSoftObjectPtr<UScriptStruct> RuntimeFrameBinding = nullptr;
	
	/// ImGui engine frame binding for rendering ImGui content for a headless instance (dedicated server).
	/// @note If you wish to handle BeginFrame and EndFrame manually, set this is None/nullptr.
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "General", meta = (ShowDisplayNames, MetaStruct = "/Script/TBImGui.TBImGuiFrameBinding_Base", ExcludeBaseStruct))
	TSoftObjectPtr<UScriptStruct> HeadlessFrameBinding = nullptr;
	
	/// Default font to use when rendering ImGui content.
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Font", meta = (GetOptions = "GetFontOptions"))
	FString DefaultFont;
	
	/// Additional font paths to scan.
	UPROPERTY(config, EditAnywhere, Category = "Font", meta = (RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> AdditionalFontPaths;
	
	/// The port used when listening for incoming NetImGui connections on a client.
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "NetImGui", meta = (DisplayName = "Port"))
	int32 NetImGuiPort = 8889;
};