// Copyright 2024 GeoTrinity. All Rights Reserved.

using UnrealBuildTool;

public class GeoTrinityUI : ModuleRules
{
	public GeoTrinityUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"SlateCore",
			"Slate",
			"UMG",
			"GameplayAbilities",
			"GameplayTags",
			"GeoTrinity"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"AdvancedSessions",
			"AdvancedSteamSessions"
		});
	}
}
