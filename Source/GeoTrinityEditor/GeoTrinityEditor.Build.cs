// Copyright 2024 GeoTrinity. All Rights Reserved.

using UnrealBuildTool;

public class GeoTrinityEditor : ModuleRules
{
	public GeoTrinityEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GeoTrinity",
			"UnrealEd",
			"Blutility",
			"SlateCore",
			"Slate",
			"UMG",
			"UMGEditor",
			"AIModule",
			"GameplayTags",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"StateTreeEditorModule",
			"PropertyBindingUtils"
		});
	}
}
