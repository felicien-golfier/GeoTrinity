// Copyright 2024 GeoTrinity. All Rights Reserved.

using UnrealBuildTool;

public class GeoTrinityEditor : ModuleRules
{
	public GeoTrinityEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"GeoTrinity",
			"GeoTrinityUI",
			"UnrealEd",
			"EditorSubsystem",
			"DirectoryWatcher",
			"GameplayAbilities",
			"Blutility",
			"NiagaraEditor",
			"SlateCore",
			"Slate",
			"UMG",
			"UMGEditor",
			"PropertyEditor",
			"RenderCore",
			"AIModule",
			"MeshDescription",
			"SkeletalMeshUtilitiesCommon",
			"AnimGraph",
			"AnimGraphRuntime",
			"BlueprintGraph",
			"GameplayTags",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"StateTreeEditorModule",
			"PropertyBindingUtils"
		});
	}
}