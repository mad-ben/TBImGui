// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <type_traits>

class UWorld;
class UTexture2D;
class UGameInstance;

#if WITH_NETIMGUI
class UTextureRenderTarget2D;
#endif

class SWidget;
class SWindow;

class FTBImGuiContext;

struct ImGuiViewport;
struct FTBImGuiContextParams;
struct FTBImGuiViewportData;

/// @brief Collection of unreal platform utility functions for ImGui related code.
class TBIMGUI_API FTBImGuiUtils
{
public:
	
	/// @brief Return the Unreal session identifier. (This is only different during PIE or multiple local clients)
	static int32 GetSessionId();
	
	/// @brief Return the Unreal ImGui config directory.
	static FString GetConfigDir();
	
	/// @brief Get the ImGui config file path.
	static FString GetImGuiConfigPath(const FString& InContextName);

	/// @brief Get the ImGui log file path.
	static FString GetImGuiLogPath(const FString& InContextName);
	
	/// @brief Get the version of the TBImGUi plugin.
	static int32 GetPluginVersion();
	
	/// @brief Return the version name of the TBImGui plugin.
	static FString GetPluginVersionName();
	
	/// @brief Returns all the default font file paths for Unreal engine.
	static TArray<FString> GetEngineFontPaths();
	
	/// @brief Copies a FString to a fixed size UTF-8 buffer, ensuring null termination. 
	/// Returns true if the entire string was copied, false if it was truncated.
	static bool CopyStringToFixedUtf8(UTF8CHAR* InDest, const int32 InDestSize, const FString& InString);
	
	/// @brief Create and return a new transient UTexture2D of the specified size and filled with the provided color.
	static UTexture2D* CreateColoredTexture(const FColor& InColor, const int32 InSizeX, const int32 InSizeY);
	
	/// @brief Static getter for the world with full PIE and standalone support.
	static UWorld* GetWorld();
	
	/// @brief Static getter for the game instance with full PIE and standalone support.
	static UGameInstance* GetGameInstance();
	
	/// @brief Static getter for the game instances runtime ImGui context.
	/// @warning This will only work if you are calling it after world initialization! Otherwise, use the game instance method below.
	static TSharedPtr<FTBImGuiContext> GetRuntimeContext();
	
	/// @brief Get the ImGui context from the game instances runtime ImGui context.
	static TSharedPtr<FTBImGuiContext> GetRuntimeContext(const UGameInstance* InGameInstance);
	
	/// @brief Create and apply an ImGui context to the given window.
	/// @note This context will need manual frame management and will not have any platform bindings set up by default.
	static TSharedPtr<FTBImGuiContext> CreateWindowContext(const TSharedRef<SWindow>& InWindow, const FTBImGuiContextParams& InContextParams);
	
	/// @brief Returns viewport data for the given window.
	static FTBImGuiViewportData* FindViewportForWindow(const TSharedPtr<SWindow>& InWindow);
	
	/// @brief Returns viewport data for the given widget.
	static FTBImGuiViewportData* FindViewportForWidget(const TSharedPtr<SWidget>& InWidget);
	
	/// @brief Enables/Disables input for all ImGui viewports.
	static void SetAllViewportInputEnabled(bool bEnableInput);
	
	/// @brief Enables/Disables input for a given ImGui viewport.
	static void SetViewportInputEnabled(const ImGuiViewport* InViewport, bool bEnableInput);
	
	/// @brief Returns the slate widget created for a given ImGuiViewport.
	template<typename T = SWidget>
	static TSharedPtr<T> GetViewportWidget(const ImGuiViewport* InViewport)
	{
		TSharedPtr<SWidget> FoundWidget = GetViewportWidgetInternal(InViewport);
		if (!FoundWidget.IsValid())
		{
			return nullptr;
		}
		
		// If the type is the same, just return.
		if constexpr (std::is_same_v<T, SWidget>)
		{
			return FoundWidget;
		}
		
		return StaticCastSharedPtr<T>(FoundWidget);
	}
	
#if WITH_IMPLOT3D
	/// @brief Extract mesh into its vertices and indices arrays for use in ImPlot3D.
	static bool ExtractMesh(const UStaticMesh* InMesh, TArray<FVector3f>& OutVertices, TArray<uint32>& OutIndices, const int32 LODIndex = 0);
#endif
	
#if WITH_NETIMGUI
	/// @brief Unreal platform function for sending a UTexture2D unreal type through NetImGui.
	static bool SendTextureToNetImGui(UTexture2D* InTexture);
	
	/// @brief Unreal platform function for sending a UTextureRenderTarget2D unreal type through NetImGui.
	static bool SendTextureToNetImGui(UTextureRenderTarget2D* InTexture);
#endif
	
private:
	
	/// @brief Returns the slate widget created for a given ImGuiViewport.
	static TSharedPtr<SWidget> GetViewportWidgetInternal(const ImGuiViewport* InViewport);
};