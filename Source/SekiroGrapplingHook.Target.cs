// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SekiroGrapplingHookTarget : TargetRules
{
	public SekiroGrapplingHookTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		ExtraModuleNames.Add("SekiroGrapplingHook");
	}
}
