// Copyright Lewis Scrivens. All Rights Reserved.

using UnrealBuildTool;

public class TBImGuiShaders : ModuleRules
{
    public TBImGuiShaders(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core"
        });
        
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "RenderCore",
            "Projects"
        });
    }
}