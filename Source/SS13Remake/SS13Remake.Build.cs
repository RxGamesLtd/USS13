using UnrealBuildTool;

public class SS13Remake : ModuleRules
{
	public SS13Remake(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore", "FluidSimulation" });
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	}
}
