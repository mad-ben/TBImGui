// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiTypes.h"

#include <Engine/GameInstance.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
#if WITH_IMPLOT
#include <implot.h>
#endif
#if WITH_IMPLOT3D
#include <implot3d.h>
#endif
THIRD_PARTY_INCLUDES_END

#include "TBImGuiContext.h"
#include "TBImGuiUtils.h"

FTBImGuiViewportData* FTBImGuiViewportData::Find(const ImGuiViewport* InViewport)
{
	return static_cast<FTBImGuiViewportData*>(InViewport->PlatformUserData);
}

FTBImGuiViewportData* FTBImGuiViewportData::FindOrCreate(ImGuiViewport* InViewport)
{
	if (FTBImGuiViewportData* const FoundViewportData = Find(InViewport))
	{
		return FoundViewportData;
	}
	
	FTBImGuiViewportData* const NewViewportData = new FTBImGuiViewportData();
	InViewport->PlatformUserData = NewViewportData;
	return NewViewportData;
}

FTBImGuiContextScope::FTBImGuiContextScope()
	: FTBImGuiContextScope(FTBImGuiUtils::GetRuntimeContext())
{
}

FTBImGuiContextScope::FTBImGuiContextScope(const UGameInstance* InGameInstance)
	: FTBImGuiContextScope(FTBImGuiUtils::GetRuntimeContext(InGameInstance))
{
}

FTBImGuiContextScope::FTBImGuiContextScope(const TSharedPtr<FTBImGuiContext>& InContext)
	: Context(InContext)
{
	PrevContext = ImGui::GetCurrentContext();
	
#if WITH_IMPLOT
	PrevPlotContext = ImPlot::GetCurrentContext();
#endif
	
#if WITH_IMPLOT3D
	PrevPlot3DContext = ImPlot3D::GetCurrentContext();
#endif
	
	if (Context.IsValid())
	{
		ImGui::SetCurrentContext(Context->GetImGuiContext());
		
#if WITH_IMPLOT
		ImPlot::SetCurrentContext(Context->GetImPlotContext());
#endif
		
#if WITH_IMPLOT3D
		ImPlot3D::SetCurrentContext(Context->GetImPlot3DContext());
#endif
	}
	else
	{
		ImGui::SetCurrentContext(nullptr);
		
#if WITH_IMPLOT
		ImPlot::SetCurrentContext(nullptr);
#endif
		
#if WITH_IMPLOT3D
		ImPlot3D::SetCurrentContext(nullptr);
#endif
	}
}

FTBImGuiContextScope::~FTBImGuiContextScope()
{
	ImGui::SetCurrentContext(PrevContext);
	
#if WITH_IMPLOT
	ImPlot::SetCurrentContext(PrevPlotContext);
#endif
	
#if WITH_IMPLOT3D
	ImPlot3D::SetCurrentContext(PrevPlot3DContext);
#endif
}

bool FTBImGuiContextScope::IsValid() const
{
	return Context.IsValid();
}

FTBImGuiContext* FTBImGuiContextScope::operator->() const
{
	return Context.operator->();
}

TSharedRef<FTBImGuiContext> FTBImGuiContextScope::GetContext() const
{
	return Context.ToSharedRef();
}