// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiShaders.h"

#include <Interfaces/IPluginManager.h>
#include <Misc/Paths.h>
#include <ShaderCore.h>

IMPLEMENT_MODULE(FTBImGuiShadersModule, TBImGuiShaders)

IMPLEMENT_GLOBAL_SHADER(FTBImGuiVS, "/Plugin/TBImGui/Private/TBImGui.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FTBImGuiPS, "/Plugin/TBImGui/Private/TBImGui.usf", "MainPS", SF_Pixel);

void FTBImGuiShadersModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TBImGui"));
	if (Plugin.IsValid())
	{
		const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/TBImGui"), ShaderDirectory);
	}
}