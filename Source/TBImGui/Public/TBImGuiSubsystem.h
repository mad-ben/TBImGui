// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Subsystems/GameInstanceSubsystem.h>

#include "TBImGuiContext.h"

#include "TBImGuiSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FTBImGuiInputDelegate, bool /* bInputEnabled */);

/// @brief ImGui implementation for runtime contexts.
UCLASS()
class TBIMGUI_API UTBImGuiSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	static const UTBImGuiSubsystem* Get();
	static UTBImGuiSubsystem* GetMutable();
	
	/// Begin USubsystem Override
	virtual bool ShouldCreateSubsystem(UObject* InOuter) const override;
	virtual void Initialize(FSubsystemCollectionBase& InCollection) override;
	virtual void Deinitialize() override;
	/// End USubsystem Override
	
	/// @brief Returns the name of the ImGui context for this game instance.
	const FString& GetContextName() const;
	
	/// @brief Returns the game instances ImGui context.
	TSharedPtr<FTBImGuiContext> GetContext() const;
	
	/// @brief Returns true if the game instance has a valid ImGui context.
	bool HasContext() const;
	
	/// Called after the ImGui context has been created.
	FSimpleMulticastDelegate& OnImGuiInitialized();
	
	/// Called after the ImGui viewport widget has been bound to the game viewport.
	FSimpleMulticastDelegate& OnImGuiViewportInitialized();
	
	/// Called before the ImGui context begins to tear down.
	FSimpleMulticastDelegate& OnImGuiShutdown();
	
#if WITH_NETIMGUI
	/// @brief Listens for remote connections.
	bool NetImGuiListen(const int32 InPort);
	
	/// @brief Connects to a remote host.
	bool NetImGuiConnect(const FString& InHost, const int32 InPort);
	
	/// @brief Closes all remote connections.
	void NetImGuiDisconnect();
	
	/// @brief Utility to check if any host is connected/listening via NetImGui.
	bool HasActiveHost() const;
	
	/// @brief Utility to check this game instance is listening/connected via NetImGui.
	bool IsHostActive() const;
	
	/// @brief Utility to check this game instance is connected via NetImGui.
	bool IsHostConnected() const;
#endif
	
protected:
	
	/// @brief Initializes the ImGui viewport for the local player.
	void InitializeImGuiViewport();
	
	/// ImGui context for the local game instance.
	TSharedPtr<FTBImGuiContext> ImGuiContext = nullptr;
	
	/// Called after the ImGui context has been created.
	FSimpleMulticastDelegate OnImGuiInitializedEvent;
	
	/// Called after the ImGui viewport widget has been bound to the game viewport.
	FSimpleMulticastDelegate OnImGuiViewportInitializedEvent;
	
	/// Called before the ImGui context begins to tear down.
	FSimpleMulticastDelegate OnImGuiShutdownEvent;
	
	FString ContextName;
	
#if WITH_NETIMGUI
	bool bConnection = false;
	FString HostName;
#endif
	
	bool bIsDedicatedServer = false;
	bool bWasCursorVisible = false;
};