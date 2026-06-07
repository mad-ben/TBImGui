// Copyright Lewis Scrivens. All Rights Reserved.

using UnrealBuildTool;

public class TBImGui : ModuleRules
{
	protected virtual bool WithImPlot => true;
	protected virtual bool WithImPlot3D => true;
	protected virtual bool WithNetImGui => true;
	
	public TBImGui(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Slate",
			"SlateCore",
			"Engine",
			"DeveloperSettings",
			"ImGuiLibrary"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"ApplicationCore",
			"InputCore",
			"RenderCore",
			"Projects",
			"RHI",
			"TBImGuiShaders"
		});

		PublicDefinitions.Add("IMGUI_USER_CONFIG=\"TBImGuiConfig.h\"");
		PublicDefinitions.Add("IMGUI_API=TBIMGUI_API");
		
		if (WithImPlot)
		{
			PublicDependencyModuleNames.Add("ImPlotLibrary");
			PublicDefinitions.Add("IMPLOT_API=IMGUI_API");
			PublicDefinitions.Add("WITH_IMPLOT=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_IMPLOT=0");
		}
		
		if (WithImPlot3D)
		{
			PublicDependencyModuleNames.Add("ImPlot3DLibrary");
			PublicDefinitions.Add("IMPLOT3D_API=IMGUI_API");
			PublicDefinitions.Add("WITH_IMPLOT3D=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_IMPLOT3D=0");
		}
		
		if (WithNetImGui)
		{
			PublicDependencyModuleNames.Add("NetImGuiLibrary");
			PublicDefinitions.Add("NETIMGUI_API=IMGUI_API");
			PublicDefinitions.Add("WITH_NETIMGUI=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_NETIMGUI=0");
		}
	}
}