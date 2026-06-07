using UnrealBuildTool;

public class ImPlot3DLibrary : ModuleRules
{
	public ImPlot3DLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicSystemIncludePaths.Add(ModuleDirectory);
	}
}