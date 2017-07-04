using UnrealBuildTool;

public class SS13Remake : ModuleRules
{
    public SS13Remake(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
            { "Core",
              "CoreUObject",
              "Engine",
              "InputCore",
              "RHI",
              "RenderCore",
              "FluidSimulation",
              "FogOfWarModule"
            });
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    }
}
