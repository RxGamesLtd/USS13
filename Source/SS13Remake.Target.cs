// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SS13RemakeTarget : TargetRules
{
    public SS13RemakeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;

        ExtraModuleNames.Add("SS13Remake");
        ExtraModuleNames.Add("FluidSimulation");
        ExtraModuleNames.Add("FogOfWarModule");
    }
}
