// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiSubsystem.h"

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
#include <imgui_internal.h>
#if WITH_NETIMGUI
#include <NetImgui_Api.h>
#endif
THIRD_PARTY_INCLUDES_END

#include "TBImGuiTypes.h"
#include "TBImGuiUtils.h"
#include "TBImGuiContext.h"
#include "TBImGuiSettings.h"
#include "Slate/SImGuiViewportWidget.h"

#if WITH_NETIMGUI
static FAutoConsoleCommandWithWorldAndArgs NetImGuiListenCommand(
	TEXT("TB.ImGui.NetImGui.Listen"),
	TEXT("Start listening for a NetImGui connections for the local instance. Args: [Port] (Optional, default: 8889)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([] (const TArray<FString>& InArgs, UWorld* InWorld)
	{
		if (UTBImGuiSubsystem* const ImGuiSubsystem = GetValid(UTBImGuiSubsystem::GetMutable()))
		{
			int32 Port = 8889;
			if (InArgs.IsValidIndex(0))
			{
				Port = FCString::Atoi(*InArgs[0]);
			}
			else
			{
				const UTBImGuiSettings* const ImGuiSettings = UTBImGuiSettings::Get();
				if (ensure(IsValid(ImGuiSettings)))
				{
					Port = ImGuiSettings->NetImGuiPort;
				}
			}
			
			ImGuiSubsystem->NetImGuiListen(Port);
		}
	})
);

static FAutoConsoleCommandWithWorldAndArgs NetImGuiConnectCommand(
	TEXT("TB.ImGui.NetImGui.Connect"),
	TEXT("Connect the local instance to a NetImGui server. Args: [IP] (Optional, default: localhost) [Port] (Optional, default: 8889)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([] (const TArray<FString>& InArgs, UWorld* InWorld)
	{
		if (UTBImGuiSubsystem* const ImGuiSubsystem = GetValid(UTBImGuiSubsystem::GetMutable()))
		{
			FString HostName = TEXT("LocalHost");
			if (InArgs.IsValidIndex(0))
			{
				HostName = InArgs[0];
			}
			
			int32 Port = 8889;
			if (InArgs.IsValidIndex(1))
			{
				Port = FCString::Atoi(*InArgs[1]);
			}
			else
			{
				const UTBImGuiSettings* const ImGuiSettings = UTBImGuiSettings::Get();
				if (ensure(IsValid(ImGuiSettings)))
				{
					Port = ImGuiSettings->NetImGuiPort;
				}
			}
					
			ImGuiSubsystem->NetImGuiConnect(HostName, Port);
		}
	})
);

static FAutoConsoleCommandWithWorld NetImGuiDisconnectCommand(
	TEXT("TB.ImGui.NetImGui.Disconnect"),
	TEXT("Disconnect any active NetImGui connection on this instance."),
	FConsoleCommandWithWorldDelegate::CreateLambda([] (UWorld* InWorld)
	{
		if (UTBImGuiSubsystem* const ImGuiSubsystem = GetValid(UTBImGuiSubsystem::GetMutable()))
		{
			ImGuiSubsystem->NetImGuiDisconnect();
		}
	})
);
#endif

const UTBImGuiSubsystem* UTBImGuiSubsystem::Get()
{
	return GetMutable();
}

UTBImGuiSubsystem* UTBImGuiSubsystem::GetMutable()
{
	UGameInstance* const GameInstance = FTBImGuiUtils::GetGameInstance();
	if (!IsValid(GameInstance))
	{
		UE_LOG(LogTBImGui, Warning, TEXT("Cannot find a valid game instance! Returning null UTBImGuiSubsystem!"));
		return nullptr;
	}
	
	return GetValid(GameInstance->GetSubsystem<UTBImGuiSubsystem>());
}

bool UTBImGuiSubsystem::ShouldCreateSubsystem(UObject* InOuter) const
{
	const UTBImGuiSettings* const ImGuiSettings = UTBImGuiSettings::Get();
	if (!ensure(IsValid(ImGuiSettings)))
	{
		return false;
	}
	
	const UWorld* const CurrentWorld = InOuter->GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return false;
	}
	
	const ENetMode NetMode = CurrentWorld->GetNetMode();
	if (!ImGuiSettings->ShouldCreateContext(NetMode))
	{
		UE_LOG(LogTBImGui, Log, TEXT("ImGui context is disabled via settings for net mode: %s, Skipping ImGui context creation..."), *ToString(NetMode));
		return false;
	}
	
	return CurrentWorld->WorldType == EWorldType::PIE || CurrentWorld->WorldType == EWorldType::Game;
}

void UTBImGuiSubsystem::Initialize(FSubsystemCollectionBase& InCollection)
{
	UGameInstance* const GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}
	
	if (const UWorld* const CurrentWorld = GetValid(GameInstance->GetWorld()))
	{
		bIsDedicatedServer = CurrentWorld->GetNetMode() == NM_DedicatedServer;
	}
	
	const UTBImGuiSettings* const ImGuiSettings = UTBImGuiSettings::Get();
	if (!ensure(IsValid(ImGuiSettings)))
	{
		return;
	}
	
	// Initialize context name for this instance.
	// @note ContextName is limited to 15 visible chars as of now so we have to be mindful of the length here.
	{
		TAnsiStringBuilder<sizeof(ImGuiContext::ContextName)> ContextNameBuilder;
		ContextNameBuilder << TEXT("Game");
		
		if (bIsDedicatedServer)
		{
			ContextNameBuilder << TEXT("_Server");
		}
		else
		{
			const int32 SessionId = FTBImGuiUtils::GetSessionId();
			if (SessionId > 0)
			{
				ContextNameBuilder << TEXT("_") << SessionId;
			}
		}
		
		ContextName = ContextNameBuilder.ToString();
	}
	
#if WITH_NETIMGUI
	// Initialize context name for this instance.
	// Could just use the ContextName, but it's better to have the extra context.
	{
		TAnsiStringBuilder<128> HostNameBuilder;
		HostNameBuilder << FApp::GetProjectName();
		
		if (bIsDedicatedServer)
		{
			HostNameBuilder << TEXT("_Server");
		}
		else
		{
			const int32 SessionId = FTBImGuiUtils::GetSessionId();
			if (SessionId > 0)
			{
				HostNameBuilder << TEXT("_") << SessionId;
			}
		}
		
		HostName = HostNameBuilder.ToString();
	}
#endif
	
	// Initialize an ImGui context for this instance.
	FTBImGuiContextParams ContextParams;
	ContextParams.ContextName = ContextName;
	ContextParams.bHeadless = bIsDedicatedServer;
	ContextParams.ViewportMode = bIsDedicatedServer ? ETBImGuiViewportMode::Single : ETBImGuiViewportMode::Multiple;
	ContextParams.ObjectUserDataWeak = GameInstance;
	ImGuiContext = FTBImGuiContext::CreateContext(ContextParams);
	check(ImGuiContext.IsValid());
	
	// Set up the default frame binding if there is one selected.
	TSoftObjectPtr<UScriptStruct> FrameBindingType = bIsDedicatedServer ? ImGuiSettings->HeadlessFrameBinding : ImGuiSettings->RuntimeFrameBinding;
	if (FrameBindingType != nullptr)
	{
		ImGuiContext->InitializeFrameBinding(FrameBindingType.LoadSynchronous());
	}
	
	// Optionally show the demo window by default.
	if (ImGuiSettings->bShowDemoWindow)
	{
		ImGuiContext->BindPreRender(FSimpleMulticastDelegate::FDelegate::CreateLambda([]
		{
			ImGui::ShowDemoWindow();
		}));
	}
	
	OnImGuiInitializedEvent.Broadcast();
	
	UE_LOG(LogTBImGui, Log, TEXT("Initialized ImGui successfully."));
	
	if (!bIsDedicatedServer)
	{
		// If we are running on a local client, wait for the viewport to exist before creating the ImGui context.
		if (IsValid(GameInstance->GetGameViewportClient()))
		{
			InitializeImGuiViewport();
		}
		else
		{
			UGameViewportClient::OnViewportCreated().AddUObject(this, &UTBImGuiSubsystem::InitializeImGuiViewport);
		}
	}
}

void UTBImGuiSubsystem::Deinitialize()
{
	UGameViewportClient::OnViewportCreated().RemoveAll(this);
	
	OnImGuiShutdownEvent.Broadcast();

#if WITH_NETIMGUI
	// Shutdown NetImGui if we have no active host or this is the instance that was active.
	if (!HasActiveHost() || IsHostActive())
	{
		NetImgui::Shutdown();
	}
#endif
	
	// Shutdown this instances context.
	if (ImGuiContext.IsValid())
	{
		ImGuiContext->DestroyContext();
		ImGuiContext.Reset();
	}
}

const FString& UTBImGuiSubsystem::GetContextName() const
{
	return ContextName;
}

TSharedPtr<FTBImGuiContext> UTBImGuiSubsystem::GetContext() const
{
	return ImGuiContext;
}

bool UTBImGuiSubsystem::HasContext() const
{
	return ImGuiContext.IsValid();
}

FDelegateHandle UTBImGuiSubsystem::BindImGuiInitialized(const FSimpleMulticastDelegate::FDelegate& InDelegate)
{
	return OnImGuiInitializedEvent.Add(InDelegate);
}

void UTBImGuiSubsystem::UnbindImGuiInitialized(const FDelegateHandle& InHandle)
{
	OnImGuiInitializedEvent.Remove(InHandle);
}

FDelegateHandle UTBImGuiSubsystem::BindImGuiViewportInitialized(const FSimpleMulticastDelegate::FDelegate& InDelegate)
{
	return OnImGuiViewportInitializedEvent.Add(InDelegate);
}

void UTBImGuiSubsystem::UnbindImGuiViewportInitialized(const FDelegateHandle& InHandle)
{
	OnImGuiViewportInitializedEvent.Remove(InHandle);
}

FDelegateHandle UTBImGuiSubsystem::BindImGuiShutdown(const FSimpleMulticastDelegate::FDelegate& InDelegate)
{
	return OnImGuiShutdownEvent.Add(InDelegate);
}

void UTBImGuiSubsystem::UnbindImGuiShutdown(const FDelegateHandle& InHandle)
{
	OnImGuiShutdownEvent.Remove(InHandle);
}

void UTBImGuiSubsystem::InitializeImGuiViewport()
{
	UGameInstance* const GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}
	
	UGameViewportClient* const LocalGameViewport = GameInstance->GetGameViewportClient();
	if (!ensureMsgf(IsValid(LocalGameViewport), TEXT("Cannot initialize ImGui game viewport instance with invalid game viewport!")))
	{
		return;
	}
	
	UGameViewportClient::OnViewportCreated().RemoveAll(this);
	
	FTBImGuiContextScope ContextScope(ImGuiContext);
	check(ContextScope.IsValid());
	
	if (FTBImGuiViewportData* const MainViewportData = FTBImGuiViewportData::FindOrCreate(ImGui::GetMainViewport()))
	{
		const TSharedRef<SImGuiViewportWidget> ImGuiViewportWidget = SNew(SImGuiViewportWidget)
			.IsOwnedByApp(true)
			.Context(ImGuiContext)
			.GameInstance(GameInstance);
		
		MainViewportData->Window = LocalGameViewport->GetWindow();
		MainViewportData->ImGuiWidget = ImGuiViewportWidget;

		LocalGameViewport->AddViewportWidgetContent(ImGuiViewportWidget, TNumericLimits<int32>::Max());
		
		UE_LOG(LogTBImGui, Log, TEXT("Added ImGui viewport widget the viewport: %s"), *LocalGameViewport->GetName());
	}
	
	OnImGuiViewportInitializedEvent.Broadcast();
}

#if WITH_NETIMGUI
bool UTBImGuiSubsystem::NetImGuiListen(const int32 InPort)
{
	FTBImGuiContextScope ContextScope(ImGuiContext);
	if (!ContextScope.IsValid())
	{
		return false;
	}
	
	if (HasActiveHost())
	{
		NetImgui::Shutdown();
	}
	
	NetImgui::Startup();
	NetImgui::ConnectFromApp(TCHAR_TO_ANSI(*HostName), InPort);
	UE_LOG(LogTBImGui, Log, TEXT("NetImGui host [%s] has started listening for a connection on port: %i"), *HostName, InPort);
	return true;
}

bool UTBImGuiSubsystem::NetImGuiConnect(const FString& InHost, const int32 InPort)
{
	FTBImGuiContextScope ContextScope(ImGuiContext);
	if (!ContextScope.IsValid())
	{
		return false;
	}
	
	if (HasActiveHost())
	{
		NetImgui::Shutdown();
	}
	
	NetImgui::Startup();
	NetImgui::ConnectToApp(TCHAR_TO_ANSI(*HostName), TCHAR_TO_ANSI(*InHost), InPort);
	UE_LOG(LogTBImGui, Log, TEXT("NetImGui host [%s] has connected to: %s:%i."), *HostName, *InHost, InPort);
	return true;
}

void UTBImGuiSubsystem::NetImGuiDisconnect()
{
	if (HasActiveHost())
	{
		NetImgui::Disconnect();
		UE_LOG(LogTBImGui, Log, TEXT("NetImGui host [%s] has disconnected."), *HostName);
	}
	
	NetImgui::Shutdown();
}

bool UTBImGuiSubsystem::HasActiveHost() const
{
	return NetImgui::IsConnected() || NetImgui::IsConnectionPending();
}

bool UTBImGuiSubsystem::IsHostActive() const
{
	if (HasActiveHost())
	{
		if (const char* ActiveClientName = NetImgui::GetActiveClientName())
		{
			if (FCStringAnsi::Strcmp(TCHAR_TO_ANSI(*HostName), ActiveClientName) == 0)
			{
				return true;
			}
		}
	}
	
	return false;
}

bool UTBImGuiSubsystem::IsHostConnected() const
{
	if (HasActiveHost())
	{
		if (const char* ActiveClientName = NetImgui::GetActiveClientName())
		{
			if (FCStringAnsi::Strcmp(TCHAR_TO_ANSI(*HostName), ActiveClientName) == 0)
			{
				return NetImgui::IsConnected();
			}
		}
	}
	
	return false;
}
#endif