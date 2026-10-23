// Copyright Lewis Scrivens. All Rights Reserved.

using UnrealBuildTool;

public class TBImGuiEditor : ModuleRules
{
    public TBImGuiEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "Slate",
            "SlateCore",
            "DeveloperSettings",
            "TBImGui"
        });
        
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Engine",
            "ImageCore",
            "Projects",
            "CoreUObject",
            "InputCore",
            "ApplicationCore",
            "ContentBrowser",
            "ContentBrowserData",
            "AssetRegistry",
            "RHI",
            "RenderCore",
            "UMG",
            "UnrealEd",
            "MainFrame",
            "OutputLog",
            "SceneOutliner",
            "PropertyEditor",
            "WorkspaceMenuStructure"
        });
    }
}
