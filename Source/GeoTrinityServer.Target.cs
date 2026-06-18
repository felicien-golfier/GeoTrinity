// Copyright 2024 GeoTrinity. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GeoTrinityServerTarget : TargetRules
{
	public GeoTrinityServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("GeoTrinity");
	}
}
