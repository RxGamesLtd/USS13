using UnrealBuildTool;

public class FluidSimulation : ModuleRules
{
    public FluidSimulation(TargetInfo Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    }
}