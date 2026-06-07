// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiContext.h"

#include <Framework/Application/SlateApplication.h>
#include <HAL/LowLevelMemTracker.h>
#include <HAL/PlatformApplicationMisc.h>
#include <HAL/PlatformProcess.h>
#include <HAL/PlatformString.h>
#include <HAL/UnrealMemory.h>
#include <UObject/Package.h>
#include <Misc/App.h>
#include <Widgets/SWindow.h>
#include <RHITypes.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
#include <imgui_internal.h>
#if WITH_IMPLOT
#include <implot.h>
#endif
#if WITH_IMPLOT3D
#include <implot3d.h>
#endif
THIRD_PARTY_INCLUDES_END

#include "Slate/SImGuiWidget.h"
#include "Slate/SImGuiWindow.h"
#include "Fonts/FontAwesome.h"
#include "TBImGuiFrameBindings.h"
#include "TBImGuiSettings.h"
#include "TBImGuiUtils.h"

FTBImGuiContextParams FTBImGuiContextParams::DefaultParams;

TMulticastDelegate<void(TSharedRef<FTBImGuiContext>)> FTBImGuiContext::OnAnyContextCreated;

static FAutoConsoleCommand PauseImGuiRenderingCommand(
	TEXT("TB.ImGui.PauseRendering"),
	TEXT("Pause rendering for the ImGui context in the game viewport."),
	FConsoleCommandDelegate::CreateLambda([]
	{
		FTBImGuiContextScope ContextScope;
		if (ContextScope.IsValid())
		{
			ContextScope->SetPauseRendering(true);
		}
	})
);

static FAutoConsoleCommand UnpauseImGuiRenderingCommand(
	TEXT("TB.ImGui.UnpauseRendering"),
	TEXT("Unpause rendering for the ImGui context in the game viewport."),
	FConsoleCommandDelegate::CreateLambda([]
	{
		FTBImGuiContextScope ContextScope;
		if (ContextScope.IsValid())
		{
			ContextScope->SetPauseRendering(false);
		}
	})
);

#pragma region ImGui Unreal Platform Implementations

static void* UnrealPlatform_MemAlloc(size_t InSize, void* InUserData)
{
	LLM_SCOPE_BYNAME(TEXT("TBImGui"));
	return FMemory::Malloc(InSize);
}

static void UnrealPlatform_MemFree(void* InPtr, void* InUserData)
{
	FMemory::Free(InPtr);
}

static void UnrealPlatform_CreateWindow(ImGuiViewport* InViewport)
{
	FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::FindOrCreate(InViewport);
	if (!ViewportData)
	{
		return;
	}
	
	TSharedPtr<FTBImGuiContext> Context = FTBImGuiContext::Get(ImGui::GetCurrentContext());
	if (!Context.IsValid())
	{
		return;
	}
	
	FTBImGuiViewportData* const MainViewportData = FTBImGuiViewportData::FindOrCreate(ImGui::GetMainViewport());
	ImGuiStyle& Style = ImGui::GetStyle();
	
	const TSharedRef<SImGuiWindow> Window = SAssignNew(ViewportData->Window, SImGuiWindow)
		.Viewport(InViewport)
		.InitialOpacity(Style.Alpha)
		.IsTopmostWindow(Context->GetContextParams().bWindowsAlwaysOnTop)
		[
			SAssignNew(ViewportData->ImGuiWidget, SImGuiWidget)
				.IsOwnedByApp(InViewport->Flags & ImGuiViewportFlags_OwnedByApp)
				.Context(Context)
				.InitialContentSize(FVector2D(InViewport->Size))
		];
	
	const TSharedPtr<SWindow> ParentWindow = MainViewportData->Window.Pin();
	if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window, ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}
}

static void UnrealPlatform_DestroyWindow(ImGuiViewport* InViewport)
{
	const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport);
	if (!ViewportData)
	{
		return;
	}
	
	if (!(InViewport->Flags & ImGuiViewportFlags_OwnedByApp))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			Window->RequestDestroyWindow();
		}
	}
	
	InViewport->PlatformUserData = nullptr;
	delete ViewportData;
}

static void UnrealPlatform_ShowWindow(ImGuiViewport* InViewport)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			Window->ShowWindow();
		}
	}
}

static void UnrealPlatform_SetWindowPos(ImGuiViewport* InViewport, ImVec2 InPosition)
{
	if (FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			Window->MoveWindowTo(FVector2f(InPosition));
		}
	}
}

static ImVec2 UnrealPlatform_GetWindowPos(ImGuiViewport* InViewport)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SImGuiWidget> ImGuiWidget = ViewportData->ImGuiWidget.Pin())
		{
			return ImGuiWidget->GetTickSpaceGeometry().GetAbsolutePosition();
		}
	}
	
	return FVector2f::ZeroVector;
}

static void UnrealPlatform_SetWindowSize(ImGuiViewport* InViewport, ImVec2 InSize)
{
	if (FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (InViewport == ImGui::GetMainViewport())
		{
			if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
			{
				Window->Resize(FVector2f(InSize));
			}
		}
		else if (const TSharedPtr<SImGuiWidget> ImGuiWidget = ViewportData->ImGuiWidget.Pin())
		{
			// We set the widgets size since our platform windows are ESizingRule::Autosized
			// This is so the resize is deferred post ImGui render, setting it directly on the window 
			// causes an instant OnPaint call which can cause artifacts and out of date draw data.
			ImGuiWidget->SetContentSize(FVector2D(InSize));
		}
	}
}

static ImVec2 UnrealPlatform_GetWindowSize(ImGuiViewport* InViewport)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SImGuiWidget> ImGuiWidget = ViewportData->ImGuiWidget.Pin())
		{
			if (InViewport == ImGui::GetMainViewport())
			{
				// The main viewport is always owned by the application, we cannot assume 
				// it is sized by its content.
				return ImGuiWidget->GetTickSpaceGeometry().GetAbsoluteSize();
			}
			
			return ImGuiWidget->GetContentSize();
		}
	}
	
	return FVector2f::ZeroVector;
}

static void UnrealPlatform_SetWindowFocus(ImGuiViewport* InViewport)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			if (const TSharedPtr<FGenericWindow> NativeWindow = Window->GetNativeWindow())
			{
				NativeWindow->BringToFront();
				NativeWindow->SetWindowFocus();
			}
		}
	}
}

static bool UnrealPlatform_GetWindowFocus(ImGuiViewport* InViewport)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			if (const TSharedPtr<FGenericWindow> NativeWindow = Window->GetNativeWindow())
			{
				return NativeWindow->IsForegroundWindow();
			}
		}
	}

	return false;
}

static bool UnrealPlatform_GetWindowMinimized(ImGuiViewport* InViewport)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			return Window->IsWindowMinimized();
		}
	}

	return false;
}

static void UnrealPlatform_SetWindowTitle(ImGuiViewport* InViewport, const char* InTitle)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			Window->SetTitle(FText::FromString(UTF8_TO_TCHAR(InTitle)));
		}
	}
}

static void UnrealPlatform_SetWindowAlpha(ImGuiViewport* InViewport, float InAlpha)
{
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			Window->SetOpacity(InAlpha);
		}
	}
}

static void UnrealPlatform_RenderWindow(ImGuiViewport* InViewport, void* InUserData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_RenderWindow);
	
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (const TSharedPtr<SImGuiWidget> ImGuiWidget = ViewportData->ImGuiWidget.Pin())
		{
			ImGuiWidget->UpdateDrawData(InViewport->DrawData);
		}
		
		// Apply any changes from the ImGui alpha style to the slate windows opacity.
		if (const TSharedPtr<SWindow> Window = ViewportData->Window.Pin())
		{
			ImGuiStyle& Style = ImGui::GetStyle();
			if (!FMath::IsNearlyEqual(Window->GetOpacity(), Style.Alpha))
			{
				Window->SetOpacity(Style.Alpha);
			}
		}
	}
}

const char* UnrealPlatform_GetClipboardText(ImGuiContext* InContext)
{
	TArray<char>* const ClipboardBuffer = static_cast<TArray<char>*>(InContext->PlatformIO.Platform_ClipboardUserData);
	if (!ClipboardBuffer)
	{
		return nullptr;
	}
	
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);
	
	const FTCHARToUTF8 Converter(*ClipboardText);
	const int32 NumBytes = Converter.Length();
	
	ClipboardBuffer->SetNumUninitialized(NumBytes + 1);
	FMemory::Memcpy(ClipboardBuffer->GetData(), Converter.Get(), NumBytes);
	(*ClipboardBuffer)[NumBytes] = '\0'; // TERMINATE.
	
	return ClipboardBuffer->GetData();
}

static void UnrealPlatform_SetClipboardText(ImGuiContext* InContext, const char* InClipboardText)
{
	FPlatformApplicationMisc::ClipboardCopy(UTF8_TO_TCHAR(InClipboardText));
}

static bool UnrealPlatform_OpenInShell(ImGuiContext* InContext, const char* InPath)
{
	return FPlatformProcess::LaunchFileInDefaultExternalApplication(UTF8_TO_TCHAR(InPath));
}

static float UnrealPlatform_GetDpiScaleAtPoint(float InX, float InY)
{
	return FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(InX, InY);
}

#pragma endregion

FTBImGuiContext::~FTBImGuiContext()
{
	DestroyContext();
}

TSharedRef<FTBImGuiContext> FTBImGuiContext::CreateContext(const FTBImGuiContextParams& InContextParams)
{
	TSharedRef<FTBImGuiContext> ContextRef = MakeShared<FTBImGuiContext>();
	ContextRef->Initialize(InContextParams);
	OnAnyContextCreated.Broadcast(ContextRef);
	return ContextRef;
}

TSharedPtr<FTBImGuiContext> FTBImGuiContext::Get(const ImGuiContext* InContext)
{
	if (InContext && InContext->IO.UserData)
	{
		// Owning instance is stored in user data for the given ImGuiContext.
		return static_cast<FTBImGuiContext*>(InContext->IO.UserData)->AsShared();
	}

	return nullptr;
}

void FTBImGuiContext::Initialize(const FTBImGuiContextParams& InContextParams)
{
	ImGui::SetAllocatorFunctions(UnrealPlatform_MemAlloc, UnrealPlatform_MemFree);

	IMGUI_CHECKVERSION();

	Context = ImGui::CreateContext();
	ContextParams = InContextParams;

#if WITH_IMPLOT
	PlotContext = ImPlot::CreateContext();
#endif
	
#if WITH_IMPLOT3D
	Plot3DContext = ImPlot3D::CreateContext();
#endif
	
	FTBImGuiContextScope ContextScope(AsShared());
	
	ImGuiIO& IO = ImGui::GetIO();
	IO.UserData = this;
	
	IO.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	IO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	IO.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
	IO.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
	IO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	IO.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	
	IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	
	if (ContextParams.bDockingSupport)
	{
		IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}
	
#if PLATFORM_DESKTOP
	if (!ContextParams.bHeadless)
	{
		// Enable multi-viewports support in any other mode than single.
		if (InContextParams.ViewportMode != ETBImGuiViewportMode::Single)
		{
			IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		}
	
		// If we are in per window viewport mode, ensure all windows are treated as separate viewports.
		if (InContextParams.ViewportMode == ETBImGuiViewportMode::PerWindow)
		{
			IO.ConfigViewportsNoAutoMerge = true;
		}
		
		IO.ConfigNavMoveSetMousePos = true;
	}
#endif
	
	// Ensure each PIE session has a uniquely identifiable context and file paths.
	{
		static constexpr int32 ImGuiContextNameSize = UE_ARRAY_COUNT(ImGuiContext::ContextName);
		if (!FTBImGuiUtils::CopyStringToFixedUtf8(reinterpret_cast<UTF8CHAR*>(Context->ContextName), ImGuiContextNameSize, *ContextParams.ContextName))
		{
			UE_LOG(LogTBImGui, Warning, TEXT("Context name '%s' is too long and will be truncated to fit the maximum length of %i characters!"), 
					*ContextParams.ContextName, ImGuiContextNameSize - 1);
		}
		
		const FString IniFilename = FTBImGuiUtils::GetImGuiConfigPath(ContextParams.ContextName);
		static constexpr int32 IniFilenameSize = UE_ARRAY_COUNT(IniFilenameUtf8);
		if (!FTBImGuiUtils::CopyStringToFixedUtf8(reinterpret_cast<UTF8CHAR*>(IniFilenameUtf8), IniFilenameSize, *IniFilename))
		{
			UE_LOG(LogTBImGui, Warning, TEXT("IniFilename '%s' is too long and will be truncated to fit the maximum length of %i characters!"), 
					*IniFilename, IniFilenameSize - 1);
		}
		IO.IniFilename = IniFilenameUtf8;
		
		const FString LogFilename = FTBImGuiUtils::GetImGuiLogPath(ContextParams.ContextName);
		static constexpr int32 LogFilenameSize = UE_ARRAY_COUNT(LogFilenameUtf8);
		if (!FTBImGuiUtils::CopyStringToFixedUtf8(reinterpret_cast<UTF8CHAR*>(LogFilenameUtf8), LogFilenameSize, *LogFilename))
		{
			UE_LOG(LogTBImGui, Warning, TEXT("LogFilename '%s' is too long and will be truncated to fit the maximum length of %i characters!"), 
					*LogFilename, LogFilenameSize - 1);
		}
		IO.LogFilename = LogFilenameUtf8;
	}
	
	// Set up Unreal platform bindings for ImGui's backend interface.
	ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
	PlatformIO.Platform_ClipboardUserData = &ClipboardBuffer;
	PlatformIO.Platform_GetClipboardTextFn = UnrealPlatform_GetClipboardText;
	PlatformIO.Platform_SetClipboardTextFn = UnrealPlatform_SetClipboardText;
	PlatformIO.Platform_OpenInShellFn = UnrealPlatform_OpenInShell;
	
	// Set up viewport related platform bindings.
	PlatformIO.Platform_CreateWindow = UnrealPlatform_CreateWindow;
	PlatformIO.Platform_DestroyWindow = UnrealPlatform_DestroyWindow;
	PlatformIO.Platform_ShowWindow = UnrealPlatform_ShowWindow;
	PlatformIO.Platform_SetWindowPos = UnrealPlatform_SetWindowPos;
	PlatformIO.Platform_GetWindowPos = UnrealPlatform_GetWindowPos;
	PlatformIO.Platform_SetWindowSize = UnrealPlatform_SetWindowSize;
	PlatformIO.Platform_GetWindowSize = UnrealPlatform_GetWindowSize;
	PlatformIO.Platform_SetWindowFocus = UnrealPlatform_SetWindowFocus;
	PlatformIO.Platform_GetWindowFocus = UnrealPlatform_GetWindowFocus;
	PlatformIO.Platform_GetWindowMinimized = UnrealPlatform_GetWindowMinimized;
	PlatformIO.Platform_SetWindowTitle = UnrealPlatform_SetWindowTitle;
	PlatformIO.Platform_SetWindowAlpha = UnrealPlatform_SetWindowAlpha;
	PlatformIO.Platform_RenderWindow = UnrealPlatform_RenderWindow;
	
	// Waiting for this to be implemented correctly...
	// PlatformIO.Platform_GetWindowDpiScale = UnrealPlatform_GetWindowDpiScale;
	
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	
	// Setup scaling so monitor DPI is considered for font/viewport sizing.
	IO.ConfigDpiScaleFonts = true;
	IO.ConfigDpiScaleViewports = true;
	
	// Set up monitor info.
	if (FSlateApplication::IsInitialized() && !InContextParams.bHeadless)
	{
		FSlateApplication& SlateApp = FSlateApplication::Get();
		if (const TSharedPtr<GenericApplication> PlatformApplication = SlateApp.GetPlatformApplication())
		{
			FDisplayMetrics DisplayMetrics;
			FDisplayMetrics::RebuildDisplayMetrics(DisplayMetrics);
			PlatformApplication->OnDisplayMetricsChanged().AddSP(this, &FTBImGuiContext::OnDisplayMetricsChanged);
			OnDisplayMetricsChanged(DisplayMetrics);
		}
	}
	else
	{
		IO.DisplaySize = ImVec2(1920, 1080);
		
		FMonitorInfo VirtualMonitorInfo;
		VirtualMonitorInfo.bIsPrimary = true;
		VirtualMonitorInfo.DisplayRect = FPlatformRect(0, 0, 1920, 1080);
		VirtualMonitorInfo.DPI = 96.0f;
		
		FDisplayMetrics DisplayMetrics;
		DisplayMetrics.MonitorInfo.Add(VirtualMonitorInfo);
		OnDisplayMetricsChanged(DisplayMetrics);
		
		UE_LOG(LogTBImGui, Log, TEXT("No slate application found. Falling back to virtual display."));
	}
	
	// Set up awesome font config for merging it onto the end of our fonts.
	ImFontConfig AwesomeFontConfig = ImFontConfig();
	AwesomeFontConfig.FontDataOwnedByAtlas = false;
	AwesomeFontConfig.FontData = (void*)GetFontAwesomeSolidData();
	AwesomeFontConfig.FontDataSize = fa_solid_900_size;
	AwesomeFontConfig.SizePixels = 14.0f;
	AwesomeFontConfig.MergeMode = true;
	ImFormatString(AwesomeFontConfig.Name, IM_COUNTOF(AwesomeFontConfig.Name), "%s, %.0fpx", "fa-solid-900.tff", AwesomeFontConfig.SizePixels);
	
	// Set up default ImGui font.
	ImFontConfig FontConfig;
	FontConfig.SizePixels = 14.0f;
	IO.Fonts->AddFontDefault(&FontConfig);
	IO.Fonts->AddFont(&AwesomeFontConfig);
	
	// Set up default platform fonts.
	const UTBImGuiSettings* const ImGuiSettings = UTBImGuiSettings::Get();
	if (IsValid(ImGuiSettings))
	{
		TArray<FString> FontPaths = FTBImGuiUtils::GetEngineFontPaths();
		FontPaths.Append(ImGuiSettings->GetAdditionalFontPaths());
		
		for (const FString& FontPath : FontPaths)
		{
			ImFont* const Font = IO.Fonts->AddFontFromFileTTF(TCHAR_TO_UTF8(*FontPath), 16.0f);
			IO.Fonts->AddFont(&AwesomeFontConfig);
			if (FPaths::GetCleanFilename(FontPath) == ImGuiSettings->DefaultFont)
			{
				IO.FontDefault = Font;
			}
		}
	}
	
#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_CUSTOM_COLOR
	// Initialize missing texture.
	{
		UTexture2D* const NewTexture = FTBImGuiUtils::CreateColoredTexture(CustomMissingTextureColor, 32.0f, 32.0f);
		CustomMissingTexture = TStrongObjectPtr<UTexture2D>(NewTexture);
		ensureMsgf(CustomMissingTexture.IsValid(), TEXT("Custom missing texture could not be created! Falling back to the Unreal default texture for missing texture resources..."));
	}
#endif
	
	// Create the main viewport data for this instance.
	FTBImGuiViewportData::FindOrCreate(ImGui::GetMainViewport());
	
	UE_LOG(LogTBImGui, Log, TEXT("Successfully initialized ImGui context: %s"), *ContextParams.ContextName);
}

void FTBImGuiContext::DestroyContext()
{
	if (FSlateApplication::IsInitialized() && !ContextParams.bHeadless)
	{
		if (const TSharedPtr<GenericApplication> PlatformApplication = FSlateApplication::Get().GetPlatformApplication())
		{
			PlatformApplication->OnDisplayMetricsChanged().RemoveAll(this);
		}
	}
	
	if (FrameBinding.IsValid())
	{
		FrameBinding.Reset();
	}
	
#if WITH_IMPLOT
	if (PlotContext)
	{
		ImPlot::DestroyContext(PlotContext);
		PlotContext = nullptr;
	}
#endif
	
#if WITH_IMPLOT3D
	if (Plot3DContext)
	{
		ImPlot3D::DestroyContext(Plot3DContext);
		Plot3DContext = nullptr;
	}
#endif
	
	if (Context)
	{
		ImGuiContext* PrevContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(Context);
		
		if (!GExitPurge)
		{
			for (ImTextureData* const TextureData : ImGui::GetPlatformIO().Textures)
			{
				UnrealPlatform_DestroyTexture(TextureData);
			}
		}
		
		ImGui::DestroyPlatformWindows();
		
#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
		// Flushing rendering commands will allow tear down of the 
		// TBImGuiCustomElement instances holding on to stale draw data.
		FlushRenderingCommands();
#endif
		
		ImGui::DestroyContext(Context);
		ImGui::SetCurrentContext(PrevContext);
		Context = nullptr;
	}
}

void FTBImGuiContext::InitializeFrameBinding(const UScriptStruct* InFrameBindingType)
{
	if (!ensureAlwaysMsgf(!FrameBinding.IsValid(), TEXT("Already have existing frame binding!")))
	{
		return;
	}
	
	if (!ensureAlwaysMsgf(IsValid(InFrameBindingType) && InFrameBindingType->IsChildOf(FTBImGuiFrameBinding_Base::StaticStruct()), 
		TEXT("Cannot initialize frame binding for context! It is null or not a valid frame binding type!")))
	{
		return;
	}
	
	FTBImGuiFrameBinding_Base FrameBindingData(AsShared());
	FrameBinding.InitializeAs(InFrameBindingType, reinterpret_cast<const uint8*>(&FrameBindingData));
	FTBImGuiFrameBinding_Base* const FrameBindingPtr = FrameBinding.GetMutablePtr<FTBImGuiFrameBinding_Base>();
	check(FrameBindingPtr);
	
	// Create frame bindings.
	FrameBindingPtr->OnBeginFrame().BindSP(this, &FTBImGuiContext::BeginFrame);
	FrameBindingPtr->OnEndFrame().BindSP(this, &FTBImGuiContext::EndFrame);
	
	UE_LOG(LogTBImGui, Verbose, TEXT("Setting up frame binding %s for context: %s"), *InFrameBindingType->GetName(), *ContextParams.ContextName);
}

void FTBImGuiContext::BeginFrame()
{
	if (bRenderingPaused || Context->WithinFrameScope)
	{
		return;
	}
	
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_BeginFrame);
	
	FTBImGuiContextScope ContextScope(AsShared());
	
	ImGuiIO& IO = ImGui::GetIO();
	IO.DeltaTime = FApp::GetDeltaTime();
	
	if (!ContextParams.bHeadless)
	{
		IO.DisplaySize = UnrealPlatform_GetWindowSize(ImGui::GetMainViewport());
		UpdatePlatformInputState();
	}
	
	ImGui::NewFrame();
	OnNewFrame.Broadcast();
}

void FTBImGuiContext::EndFrame()
{
	if (!Context->WithinFrameScope)
	{
		return;
	}
	
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_EndFrame);
	
	FTBImGuiContextScope ContextScope(AsShared());
	
	OnPreRender.Broadcast();
	
	ImGui::Render();
	
	UpdatePlatformTextures();
	UnrealPlatform_RenderWindow(ImGui::GetMainViewport(), nullptr);
	
	// Only update platform windows if our current frame is rendering with multi-viewport support.
	if (Context->ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable)
	{
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_UpdatePlatformWindows);
			ImGui::UpdatePlatformWindows();
		}
		
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_RenderPlatformWindowsDefault);
			ImGui::RenderPlatformWindowsDefault();
		}
	}
	
	OnPostRender.Broadcast();
}

void FTBImGuiContext::SetPauseRendering(bool bPause)
{
	bRenderingPaused = bPause;
}

FDelegateHandle FTBImGuiContext::BindNewFrame(const FSimpleMulticastDelegate::FDelegate& InDelegate)
{
	return OnNewFrame.Add(InDelegate);
}

void FTBImGuiContext::UnbindNewFrame(const FDelegateHandle& InHandle)
{
	OnNewFrame.Remove(InHandle);
}

FDelegateHandle FTBImGuiContext::BindPreRender(const FSimpleMulticastDelegate::FDelegate& InDelegate)
{
	return OnPreRender.Add(InDelegate);
}

void FTBImGuiContext::UnbindPreRender(const FDelegateHandle& InHandle)
{
	OnPreRender.Remove(InHandle);
}

FDelegateHandle FTBImGuiContext::BindPostRender(const FSimpleMulticastDelegate::FDelegate& InDelegate)
{
	return OnPostRender.Add(InDelegate);
}

void FTBImGuiContext::UnbindPostRender(const FDelegateHandle& InHandle)
{
	OnPostRender.Remove(InHandle);
}

ImGuiContext* FTBImGuiContext::GetImGuiContext() const
{
	return Context;
}

#if WITH_IMPLOT
ImPlotContext* FTBImGuiContext::GetImPlotContext() const
{
	return PlotContext;
}
#endif

#if WITH_IMPLOT3D
ImPlot3DContext* FTBImGuiContext::GetImPlot3DContext() const
{
	return Plot3DContext;
}
#endif

const FTBImGuiContextParams& FTBImGuiContext::GetContextParams() const
{
	return ContextParams;
}

void FTBImGuiContext::SetInputMode(const ETBImGuiInputMode InMode, bool bFlushInput)
{
	FTBImGuiContextScope ContextScope(AsShared());
	
	if (bFlushInput)
	{
		ImGui::FlushInput();
	}
	
	if (InputMode == InMode)
	{
		return;
	}
	
	UE_LOG(LogTBImGui, Verbose, TEXT("Updating input mode for context: %s from %s to %s"), *ContextParams.ContextName, 
		*UEnum::GetValueAsString(InputMode), *UEnum::GetValueAsString(InMode));
	
	InputMode = InMode;
	
	switch (InputMode)
	{
		case ETBImGuiInputMode::All:
		{
			FTBImGuiUtils::SetAllViewportInputEnabled(true);
		}
		break;
		case ETBImGuiInputMode::MainViewportOnly:
		{
			FTBImGuiUtils::SetAllViewportInputEnabled(false);
			FTBImGuiUtils::SetViewportInputEnabled(ImGui::GetMainViewport(), true);
		}
		break;
		case ETBImGuiInputMode::ViewportsOnly:
		{
			FTBImGuiUtils::SetAllViewportInputEnabled(true);
			FTBImGuiUtils::SetViewportInputEnabled(ImGui::GetMainViewport(), false);
		}
		break;
		case ETBImGuiInputMode::None:
		{
			FTBImGuiUtils::SetAllViewportInputEnabled(false);
		}
		break;
		default: break;
	}
}

ETBImGuiInputMode FTBImGuiContext::GetInputMode() const
{
	return InputMode;
}

bool FTBImGuiContext::HasActiveFrame() const
{
	return Context->WithinFrameScope;
}

#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_CUSTOM_COLOR
UTexture2D* FTBImGuiContext::GetCustomMissingTexture()
{
	return CustomMissingTexture.Get();
}
#endif

UTexture2D* FTBImGuiContext::GetMissingTexture()
{
#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_CUSTOM_COLOR
	return GetCustomMissingTexture();
#elif IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_ENGINE_DEFAULT
	return GEngine ? GEngine->DefaultTexture : nullptr;
#else 
	return nullptr;
#endif
}

void FTBImGuiContext::OnDisplayMetricsChanged(const FDisplayMetrics& InDisplayMetrics)
{
	FTBImGuiContextScope ContextScope(AsShared());
	
	ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
	PlatformIO.Monitors.resize(0);

	for (const FMonitorInfo& Monitor : InDisplayMetrics.MonitorInfo)
	{
		ImGuiPlatformMonitor ImGuiMonitor;
		
		ImGuiMonitor.MainPos = FIntPoint(Monitor.DisplayRect.Left, Monitor.DisplayRect.Top);
		ImGuiMonitor.MainSize = FIntPoint(Monitor.DisplayRect.Right - Monitor.DisplayRect.Left, Monitor.DisplayRect.Bottom - Monitor.DisplayRect.Top);
		ImGuiMonitor.WorkPos = FIntPoint(Monitor.WorkArea.Left, Monitor.WorkArea.Top);
		ImGuiMonitor.WorkSize = FIntPoint(Monitor.WorkArea.Right - Monitor.WorkArea.Left, Monitor.WorkArea.Bottom - Monitor.WorkArea.Top);
		ImGuiMonitor.DpiScale = UnrealPlatform_GetDpiScaleAtPoint(	
			Monitor.DisplayRect.Left + (Monitor.DisplayRect.Right  - Monitor.DisplayRect.Left) * 0.5f,
			Monitor.DisplayRect.Top  + (Monitor.DisplayRect.Bottom - Monitor.DisplayRect.Top ) * 0.5f);
		
		if (Monitor.bIsPrimary)
		{
			PlatformIO.Monitors.push_front(ImGuiMonitor);
		}
		else
		{
			PlatformIO.Monitors.push_back(ImGuiMonitor);
		}
	}
}

void FTBImGuiContext::UpdatePlatformTextures()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_UpdatePlatformTextures);
	
	for (ImTextureData* const TextureData : ImGui::GetPlatformIO().Textures)
	{
		if (TextureData->Status == ImTextureStatus_WantCreate)
		{
			UnrealPlatform_CreateTexture(TextureData);
		}
		else if (TextureData->Status == ImTextureStatus_WantUpdates)
		{
			UnrealPlatform_UpdateTexture(TextureData);
		}
		else if (TextureData->Status == ImTextureStatus_WantDestroy && TextureData->UnusedFrames > 0)
		{
			UnrealPlatform_DestroyTexture(TextureData);
		}
	}
}

void FTBImGuiContext::UnrealPlatform_CreateTexture(ImTextureData* InTextureData)
{
	const FName TextureName(WriteToString<32>("ImGuiTexture_", InTextureData->UniqueID));
	
	UTexture2D* const Texture = UTexture2D::CreateTransient(
			InTextureData->Width, InTextureData->Height, PF_B8G8R8A8,
			MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), TextureName),
			MakeConstArrayView(static_cast<uint8*>(InTextureData->GetPixels()), InTextureData->GetSizeInBytes())
		);
	
	InTextureData->SetTexID(Texture);
	
	// Hold onto the texture reference until ImGui is done with it to prevent GC from collecting it.
	ImGuiTextures.Emplace(Texture);
	
	InTextureData->SetStatus(ImTextureStatus_OK);
	
	// This fixes flicker when resizing fonts.
	// ImGui may submit draw commands referencing this texture on the next frame.
	// Texture creation is rare enough that blocking here should not cause any performance issues.
	FlushRenderingCommands();
	
	UE_LOG(LogTBImGui, VeryVerbose, TEXT("Created ImGui texture: %s"), *TextureName.ToString());
}

void FTBImGuiContext::UnrealPlatform_UpdateTexture(ImTextureData* InTextureData)
{
	UTexture2D* const Texture = Cast<UTexture2D>(InTextureData->GetTexID());
	checkf(IsValid(Texture), TEXT("Trying to update invalid texture! Lifetime has been mismanaged!"));

	const int32 NumRegions = InTextureData->Updates.size();
	FUpdateTextureRegion2D* const Regions = static_cast<FUpdateTextureRegion2D*>(FMemory::Malloc(sizeof(FUpdateTextureRegion2D) * NumRegions));
	
	for (int32 RegionIdx = 0; RegionIdx < NumRegions; ++RegionIdx)
	{
		const ImTextureRect& Rect = InTextureData->Updates[RegionIdx];

		FUpdateTextureRegion2D& Region = Regions[RegionIdx];
		Region.DestX = Region.SrcX = Rect.x;
		Region.DestY = Region.SrcY = Rect.y;
		Region.Width = Rect.w;
		Region.Height = Rect.h;
	}

	Texture->UpdateTextureRegions(
		0, NumRegions, Regions,
		InTextureData->GetPitch(), InTextureData->BytesPerPixel,
		static_cast<uint8*>(InTextureData->GetPixels()),
		[](uint8*, const FUpdateTextureRegion2D* Regions)
		{
			FMemory::Free(const_cast<FUpdateTextureRegion2D*>(Regions));
		}
	);

	InTextureData->SetStatus(ImTextureStatus_OK);
}

void FTBImGuiContext::UnrealPlatform_DestroyTexture(ImTextureData* InTextureData)
{
	UTexture2D* const Texture = Cast<UTexture2D>(InTextureData->GetTexID());
	checkf(IsValid(Texture), TEXT("Invalid texture being destroyed! Lifetime has been mismanaged!"));
	
#if !NO_LOGGING
	const FString TextureName = Texture->GetName();
#endif
	
	// Release the texture resource immediately but let GC clean up the object
	Texture->ReleaseResource();
	
	// Release our hold on the texture reference to allow GC to clean it up.
	ImGuiTextures.RemoveAllSwap([Texture](const TStrongObjectPtr<UTexture>& TextureReference)
	{
		return TextureReference.Get() == Texture;
	});
	
	InTextureData->SetTexID(ImTextureID_Invalid);
	InTextureData->SetStatus(ImTextureStatus_Destroyed);
	
	UE_LOG(LogTBImGui, VeryVerbose, TEXT("Destroyed ImGui texture: %s"), *TextureName);
}

void FTBImGuiContext::UpdatePlatformInputState()
{
	if (InputMode == ETBImGuiInputMode::None)
	{
		return;
	}
	
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_UpdatePlatformInputState);
	
	FTBImGuiContextScope ContextScope(AsShared());
	
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication& SlateApp = FSlateApplication::Get();
		
		ImGuiIO& IO = ImGui::GetIO();
		
		// Refresh gamepad connection state from the slate app.
		if (SlateApp.IsGamepadAttached())
		{
			IO.BackendFlags |= ImGuiBackendFlags_HasGamepad;
		}
		else
		{
			IO.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
		}
		
		// Refresh mouse state.
		const bool bImGuiMouseActive = (IO.ConfigFlags & ImGuiConfigFlags_NoMouse) == 0;
		if (bImGuiMouseActive && SlateApp.IsMouseAttached())
		{
			// Apply Slate -> ImGui mouse state.
			if (SlateApp.GetCursorUser()->IsCursorVisible())
			{
				const int32 UserIndex = SlateApp.GetCursorUser()->GetUserIndex();
				const TSharedPtr<FSlateUser> SlateUser = SlateApp.GetUser(UserIndex);
				
				if (SlateUser.IsValid())
				{
					// Check if we have any ImGui viewports under the mouse that are accepting input.
					bool bViewportAcceptsInput = false;
					const FWeakWidgetPath LastWidgetsUnderPointer = SlateUser->GetLastWidgetsUnderPointer(FSlateApplicationBase::CursorPointerIndex);
					const FTBImGuiViewportData* const ViewportFromViewport = FTBImGuiUtils::FindViewportForWindow(LastWidgetsUnderPointer.Window.Pin());
					if (ViewportFromViewport && ViewportFromViewport->ImGuiWidget.IsValid())
					{
						bViewportAcceptsInput = ViewportFromViewport->ImGuiWidget.Pin()->IsInputEnabled();
					}
					else
					{
						for (const TWeakPtr<SWidget>& Widget : LastWidgetsUnderPointer.Widgets)
						{
							if (Widget.IsValid())
							{
								const FTBImGuiViewportData* const ViewportFromWidget = FTBImGuiUtils::FindViewportForWidget(Widget.Pin());
								if (ViewportFromWidget && ViewportFromWidget->ImGuiWidget.IsValid())
								{
									bViewportAcceptsInput = ViewportFromWidget->ImGuiWidget.Pin()->IsInputEnabled();
									break;
								}
							}
						}
					}
					
					// Only update mouse position events if there is a valid ImGui widget under 
					// the mouse that is actively accepting input. Or we have an ImGui drag context.
					if (bViewportAcceptsInput || ImGui::IsMouseDown(ImGuiMouseButton_Left))
					{
						FVector2f SlateMousePosition = SlateApp.GetCursorPos();
						if (!(IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
						{
							SlateMousePosition -= ImGui::GetScreenSpaceAbsolutePosition();
						}

						IO.AddMousePosEvent(SlateMousePosition.X, SlateMousePosition.Y);
					}
					// Otherwise clear the mouse position info so elements are un-hovered.
					else
					{
						IO.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
					}
				}
			}
			
			// Apply ImGui -> Slate mouse state.
			if (IO.WantSetMousePos)
			{
				FVector2f NewPosition = IO.MousePos;
				if (!(IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
				{
					NewPosition += ImGui::GetScreenSpaceAbsolutePosition();
				}
				
				SlateApp.GetCursorUser()->SetCursorPosition(NewPosition);
			}
		}
	}
}