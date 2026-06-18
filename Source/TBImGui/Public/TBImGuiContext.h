// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Templates/SharedPointer.h>
#include <UObject/StrongObjectPtr.h>
#include <StructUtils/InstancedStruct.h>
#include <Engine/Texture2D.h>

#include "TBImGuiConfig.h"
#include "TBImGuiTypes.h"

class FTBImGuiContext;

struct FDisplayMetrics;

struct ImGuiContext;
struct ImGuiViewport;
struct ImTextureData;

#if WITH_IMPLOT
struct ImPlotContext;
#endif

#if WITH_IMPLOT3D
struct ImPlot3DContext;
#endif

UENUM()
enum class ETBImGuiViewportMode : uint8
{
	Single,		// Only the main viewport will be considered.
	Multiple,	// Windows can be dragged out of the main viewport.
	PerWindow	// All windows are assigned their own viewport.
};

struct FTBImGuiContextParams
{
	static FTBImGuiContextParams DefaultParams;
	
	FTBImGuiContextParams() = default;
	FTBImGuiContextParams(const FString& InContextName)
		: ContextName(InContextName)
	{
	}
	
	/// The ImGui name for the context.
	/// @note Limited to 15 characters!
	FString ContextName;
	
	/// What viewport support does this context have.
	ETBImGuiViewportMode ViewportMode = ETBImGuiViewportMode::Multiple;
	
	/// Does this context have docking support?
	bool bDockingSupport = true;
	
	/// Is this context headless? Has no slate context?
	bool bHeadless = false;
	
	/// Should external viewport windows always render on-top.
	bool bWindowsAlwaysOnTop = false;
	
	/// Additional raw data for this context.
	void* RawUserData = nullptr;
	
	/// Additional user UObject data for this context.
	TWeakObjectPtr<UObject> ObjectUserDataWeak = nullptr;
};

/// @brief ImGui platform context for Unreal Engine bindings.
class TBIMGUI_API FTBImGuiContext : public TSharedFromThis<FTBImGuiContext>
{
public:
	
	~FTBImGuiContext();
	
	/// @brief Instance and return a new ImGui context.
	static TSharedRef<FTBImGuiContext> CreateContext(const FTBImGuiContextParams& InContextParams);
	
	/// @brief Returns an existing unreal ImGui context for a given ImGui internal context.
	static TSharedPtr<FTBImGuiContext> Get(const ImGuiContext* InContext);
	
	/// @brief Destroy this context instance, close all platform windows and release all ImGui resources.
	void DestroyContext();
	
	/// @brief Initialize the frame binding mode for the ImGui context. 
	/// @note This will set up bindings to call BeginFrame and EndFrame.
	void InitializeFrameBinding(const UScriptStruct* InFrameBindingType);
	
	/// @brief Start a new ImGui frame scope for this context until EndFrame is called.
	/// @note Any input processed after this point will not take effect until the next frame.
	void BeginFrame();
	
	/// @brief Called at the end of the ImGui frame to render the draw data and 
	/// update ImGui texture states for the unreal platform.
	void EndFrame();
	
	/// @brief Start/Stop ImGui render loop.
	void SetPauseRendering(bool bPause);
	
	/// @brief Event called when a new ImGui frame begins for this instance.
	FSimpleMulticastDelegate& OnNewFrame();
	
	/// @brief Event called right before ImGui is rendered for this instance.
	FSimpleMulticastDelegate& OnPreRender();
	
	/// @brief Event called right after ImGui is rendered for this instance.
	FSimpleMulticastDelegate& OnPostRender();
	
	/// @brief Return the underlying ImGui context.
	ImGuiContext* GetImGuiContext() const;
	
#if WITH_IMPLOT
	/// @brief Return the underlying ImPlot context.
	ImPlotContext* GetImPlotContext() const;
#endif
	
#if WITH_IMPLOT3D
	/// @brief Return the underlying ImPlot3D context.
	ImPlot3DContext* GetImPlot3DContext() const;
#endif
	
	/// @brief Returns the cached context parameters used when the ImGui context was initialized.
	const FTBImGuiContextParams& GetContextParams() const;
	
	/// @brief Set the input mode for all viewports owned by this context.
	/// @note See definitions for ETBImGuiInputMode.
	void SetInputMode(const ETBImGuiInputMode InMode, bool bFlushInput = true);
	
	/// @brief Returns the active input mode for this context.
	ETBImGuiInputMode GetInputMode() const;

	/// @brief Returns true if the ImGui context has an active frame scope, false otherwise.
	bool HasActiveFrame() const;
	
#if IMGUI_MISSING_TEXTURE_MODE == 2
	UTexture2D* GetCustomMissingTexture();
#endif
	
	/// @brief Returns the missing texture resource configured in TBImGuiConfig.
	UTexture2D* GetMissingTexture();
	
	/// Callback for when any ImGui context has been created.
	static TMulticastDelegate<void(TSharedRef<FTBImGuiContext>)> OnAnyContextCreated;
	
private:
	
	/// @brief Callback for when display settings change to update 
	/// ImGui's internal state for active displays.
	void OnDisplayMetricsChanged(const FDisplayMetrics& InDisplayMetrics);
	
	/// @brief Handles all platform texture requests from ImGui.
	void UpdatePlatformTextures();
	
	/// @brief Creates an ImGui managed texture from the provided ImTextureData.
	void UnrealPlatform_CreateTexture(ImTextureData* InTextureData);
	
	/// @brief Updates an ImGui managed texture from the provided ImTextureData.
	void UnrealPlatform_UpdateTexture(ImTextureData* InTextureData);
	
	/// @brief Destroys and releases an ImGui managed texture from the provided ImTextureData.
	void UnrealPlatform_DestroyTexture(ImTextureData* InTextureData);
	
	/// @brief Poll slate input state and update ImGui's internal input state to match.
	void UpdatePlatformInputState();
	
protected:
	
	/// @brief Initializes the ImGui context, configures platform and unreal platform renderer bindings, 
	/// fonts, and registers frame delegates.
	void Initialize(const FTBImGuiContextParams& InContextParams);
	
	ImGuiContext* Context = nullptr;
	
	FInstancedStruct FrameBinding;

#if WITH_IMPLOT
	ImPlotContext* PlotContext = nullptr;
#endif
	
#if WITH_IMPLOT3D
	ImPlot3DContext* Plot3DContext = nullptr;
#endif
	
	/// Store for ImGui created/managed textures.
	TArray<TStrongObjectPtr<UTexture2D>> ImGuiTextures;
	
#if IMGUI_MISSING_TEXTURE_MODE == 2
	TStrongObjectPtr<UTexture2D> CustomMissingTexture = nullptr;
#endif
	
	FTBImGuiContextParams ContextParams;
	
	ETBImGuiInputMode InputMode = ETBImGuiInputMode::All;
	
	bool bRenderingPaused = false;
	
	char IniFilenameUtf8[512] = {};
	char LogFilenameUtf8[512] = {};
	
	TArray<char> ClipboardBuffer;
	
	FSimpleMulticastDelegate OnNewFrameEvent;
	FSimpleMulticastDelegate OnPreRenderEvent;
	FSimpleMulticastDelegate OnPostRenderEvent;
};