// Copyright 2015-2015 RxGames, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FluidSimulation : ModuleRules
{
    public FluidSimulation(TargetInfo Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
    }
}