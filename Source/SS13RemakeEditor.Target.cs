// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SS13RemakeEditorTarget : TargetRules
{
    public SS13RemakeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        ExtraModuleNames.Add("SS13Remake");
        ExtraModuleNames.Add("FluidSimulation");
        ExtraModuleNames.Add("FogOfWarModule");
    }
}
