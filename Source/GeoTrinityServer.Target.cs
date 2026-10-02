// Copyright 2024 GeoTrinity. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GeoTrinityServerTarget : TargetRules
{
	public GeoTrinityServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GeoTrinity");
	}
}
