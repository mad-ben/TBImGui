// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiFrameBindings.h"

#include <Misc/CoreDelegates.h>
#include <Framework/Application/SlateApplication.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <UObject/Object.h>

#include "TBImGuiSubsystem.h"

FTBImGuiFrameBinding_Application::FTBImGuiFrameBinding_Application()
{
	FCoreDelegates::OnBeginFrame.AddRaw(this, &FTBImGuiFrameBinding_Application::OnApplicationBeginFrame);
	
	if (ensureMsgf(FSlateApplication::IsInitialized(), TEXT("Cannot create frame binding if slate is not yet initialized!")))
	{
		FSlateApplication::Get().OnPreTick().AddRaw(this, &FTBImGuiFrameBinding_Application::OnSlatePreTick);
	}
}

FTBImGuiFrameBinding_Application::~FTBImGuiFrameBinding_Application()
{
	if (FCoreDelegates::OnBeginFrame.IsBoundToObject(this))
	{
		FCoreDelegates::OnBeginFrame.RemoveAll(this);
	}
	
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnPreTick().RemoveAll(this);
	}
}

void FTBImGuiFrameBinding_Application::OnApplicationBeginFrame()
{
	if (OnBeginFrameEvent.IsBound())
	{
		OnBeginFrameEvent.Execute();
	}
}

void FTBImGuiFrameBinding_Application::OnSlatePreTick(float InDeltaSeconds)
{
	if (OnEndFrameEvent.IsBound())
	{
		OnEndFrameEvent.Execute();
	}
}

FTBImGuiFrameBinding_Headless::FTBImGuiFrameBinding_Headless()
{
	FCoreDelegates::OnEndFrame.AddRaw(this, &FTBImGuiFrameBinding_Headless::OnApplicationEndFrame);
}

FTBImGuiFrameBinding_Headless::~FTBImGuiFrameBinding_Headless()
{
	if (FCoreDelegates::OnEndFrame.IsBoundToObject(this))
	{
		FCoreDelegates::OnEndFrame.RemoveAll(this);
	}
}

void FTBImGuiFrameBinding_Headless::OnApplicationEndFrame()
{
	if (OnBeginFrameEvent.IsBound())
	{
		OnBeginFrameEvent.Execute();
	}
	
	if (OnEndFrameEvent.IsBound())
	{
		OnEndFrameEvent.Execute();
	}
}

FTBImGuiFrameBinding_Slate::FTBImGuiFrameBinding_Slate()
{
	if (ensureMsgf(FSlateApplication::IsInitialized(), TEXT("Cannot create frame binding if slate is not yet initialized!")))
	{
		FSlateApplication::Get().OnPreTick().AddRaw(this, &FTBImGuiFrameBinding_Slate::OnSlatePreTick);
	}
}

FTBImGuiFrameBinding_Slate::~FTBImGuiFrameBinding_Slate()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnPreTick().RemoveAll(this);
	}
}

void FTBImGuiFrameBinding_Slate::OnSlatePreTick(float InDeltaSeconds)
{
	if (OnBeginFrameEvent.IsBound())
	{
		OnBeginFrameEvent.Execute();
	}
	
	if (OnEndFrameEvent.IsBound())
	{
		OnEndFrameEvent.Execute();
	}
}

FTBImGuiFrameBinding_World::FTBImGuiFrameBinding_World()
{
	FWorldDelegates::OnWorldPostActorTick.AddRaw(this, &FTBImGuiFrameBinding_World::WorldPostActorTick);
}

FTBImGuiFrameBinding_World::~FTBImGuiFrameBinding_World()
{
	if (FWorldDelegates::OnWorldPostActorTick.IsBoundToObject(this))
	{
		FWorldDelegates::OnWorldPostActorTick.RemoveAll(this);
	}
}

void FTBImGuiFrameBinding_World::WorldPostActorTick(UWorld* InWorld, ELevelTick InTickType, float InDeltaSeconds)
{
	if (!IsValid(InWorld) || !InWorld->IsGameWorld())
	{
		return;
	}
	
	if (!ensureMsgf(OwningContext.IsValid(), TEXT("Cannot use world frame binding without valid context!")))
	{
		return;
	}
	
	UGameInstance* const GameInstance = Cast<UGameInstance>(OwningContext.Pin()->GetContextParams().ObjectUserDataWeak.Get());
	if (!ensureMsgf(IsValid(GameInstance), TEXT("Cannot use world frame binding outside a runtime context! No game instance found!")))
	{
		return;
	}
		
	// Only tick the ImGui context on the world it belongs to.
	if (GameInstance == InWorld->GetGameInstance())
	{
		if (OnBeginFrameEvent.IsBound())
		{
			OnBeginFrameEvent.Execute();
		}
		
		if (OnEndFrameEvent.IsBound())
		{
			OnEndFrameEvent.Execute();
		}
	}
}