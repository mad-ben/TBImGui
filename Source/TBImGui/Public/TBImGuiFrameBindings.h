// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Delegates/Delegate.h>
#include <Engine/EngineBaseTypes.h>

#include "TBImGuiFrameBindings.generated.h"

class UWorld;

class FTBImGuiContext;

/// @brief Base frame binding type for ImGui contexts. 
/// Any call of OnBeginFrameEvent will execute its owning contexts BeginFrame function.
/// Any call of OnEndFrameEvent will execute its owning contexts End function.
USTRUCT()
struct TBIMGUI_API FTBImGuiFrameBinding_Base
{
	GENERATED_BODY()
	
	FTBImGuiFrameBinding_Base() = default;
	FTBImGuiFrameBinding_Base(const TSharedPtr<FTBImGuiContext>& OwningContext)
		: OwningContext(OwningContext.ToWeakPtr())
	{}
	
	FORCEINLINE FSimpleDelegate& OnBeginFrame()
	{
		return OnBeginFrameEvent;
	}
	
	FORCEINLINE FSimpleDelegate& OnEndFrame()
	{
		return OnEndFrameEvent;
	}
	
protected:
	
	TWeakPtr<FTBImGuiContext> OwningContext;
	
	FSimpleDelegate OnBeginFrameEvent;
	FSimpleDelegate OnEndFrameEvent;
};

/// @brief ImGui BeginFrame is run on FApp::BeginFrame and ImGui EndFrame is run on FSlateApplication::OnPreTick (Right before rendering the ImGui content).
/// @note This allows an ImGui contexts and content to be used at anypoint in the application scope.
/// @note Caveat: The input is updated on BeginFrame before the engine has polled the OS for new input. This causes things like Cursor icon queries failing and 1 frame of input lag.
USTRUCT(meta = (DisplayName = "Application Frame Binding"))
struct TBIMGUI_API FTBImGuiFrameBinding_Application :  public FTBImGuiFrameBinding_Base
{
	GENERATED_BODY()
	
	FTBImGuiFrameBinding_Application();
	~FTBImGuiFrameBinding_Application();
	
protected:
	
	void OnApplicationBeginFrame();
	void OnSlatePreTick(float InDeltaSeconds);
};

/// @brief ImGui BeginFrame and EndFrame is run on on FApp::EndFrame.
/// @note This is specifically for headless applications since it does not rely on a valid slate application.
USTRUCT(meta = (DisplayName = "Headless Frame Binding"))
struct TBIMGUI_API FTBImGuiFrameBinding_Headless :  public FTBImGuiFrameBinding_Base
{
	GENERATED_BODY()
	
	FTBImGuiFrameBinding_Headless();
	~FTBImGuiFrameBinding_Headless();
	
protected:
	
	void OnApplicationEndFrame();
};

/// @brief Both ImGui BeginFrame and EndFrame are run on FSlateApplication::OnPreTick. 
/// @note This is the best out-of-the-box solution with minimal drawbacks.
USTRUCT(meta = (DisplayName = "Slate Frame Binding"))
struct TBIMGUI_API FTBImGuiFrameBinding_Slate :  public FTBImGuiFrameBinding_Base
{
	GENERATED_BODY()
	
	FTBImGuiFrameBinding_Slate();
	~FTBImGuiFrameBinding_Slate();
	
protected:
	
	void OnSlatePreTick(float InDeltaSeconds);
};

/// @brief Both ImGui BeginFrame and EndFrame are run on FWorldDelegates::OnWorldPostActorTick.
/// @note Caveat: The ImGui will not render until a valid world has ticked. (World transitions, game instance startup etc.)
USTRUCT(meta = (DisplayName = "World Frame Binding"))
struct TBIMGUI_API FTBImGuiFrameBinding_World :  public FTBImGuiFrameBinding_Base
{
	GENERATED_BODY()
	
	FTBImGuiFrameBinding_World();
	~FTBImGuiFrameBinding_World();
	
protected:
	
	void WorldPostActorTick(UWorld* InWorld, ELevelTick InTickType, float InDeltaSeconds);
};