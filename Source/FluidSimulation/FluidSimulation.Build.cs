using UnrealBuildTool;

public class FluidSimulation : ModuleRules
{
    public FluidSimulation(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
            { "Core",
              "CoreUObject",
              "Engine",
              "InputCore"
            });
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    }
}