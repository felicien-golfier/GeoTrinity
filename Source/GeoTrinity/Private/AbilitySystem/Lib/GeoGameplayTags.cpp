#include "AbilitySystem/Lib/GeoGameplayTags.h"

#include "GameplayTagsManager.h"


FGeoGameplayTags FGeoGameplayTags::GameplayTags;
bool FGeoGameplayTags::bNativeTagsInitialized = false;

namespace
{
	/** Registers a native tag (survives editor restarts without a .ini entry) and stores it in the provided slot. */
	void CreateAndAssignGameplayTag(FGameplayTag& outTag, FName const& tagName, FString const& comment)
	{
		outTag = UGameplayTagsManager::Get().AddNativeGameplayTag(tagName, comment);
	}

	/** Prepends the shared InputTag root so call sites only specify the leaf name. */
	void AddInputTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::InputTag + "." + tagName), comment);
	}

	/** Prepends the shared AbilityTypeTag root so call sites only specify the leaf name. */
	void AddAbilityTypeTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::AbilityTypeTag + "." + tagName), comment);
	}

	/** Prepends the shared AbilitySpellTag root so call sites only specify the leaf name. */
	void AddAbilitySpellTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::AbilitySpellTag + "." + tagName), comment);
	}
} // namespace

void FGeoGameplayTags::InitializeNativeGameplayTags()
{
	bNativeTagsInitialized = true;
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Damage, "Gameplay.Damage", "Tag to identify damage");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Heal, "Gameplay.Heal", "Tag to identify healing");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Shield, "Gameplay.Shield", "Tag to identify shield");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_DurationMagnitude, "Gameplay.DurationMagnitude",
							   "SetByCaller magnitude tag for Duration Magnitude");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_GenericMagnitude, "Gameplay.GenericMagnitude",
							   "SetByCaller magnitude tag for Generic Gameplay Effects.");

	// INPUT //
	AddInputTag(GameplayTags.InputTag_Basic, "Basic", "Input tag for left mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_Special, "Special", "Input tag for right mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_SpecialAlternative, "SpecialAlternative",
				"Input tag for alternative special ability");
	AddInputTag(GameplayTags.InputTag_Reload, "Reload", "Input tag for reload action");
	AddInputTag(GameplayTags.InputTag_Dash, "Dash", "Input tag for dash ability");

	// ABILITY TYPES //
	CreateAndAssignGameplayTag(GameplayTags.Ability_Type, FName(RootTagNames::AbilityTypeTag),
							   "Root tag for ability types");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Basic, "Basic", "Tag associated with basic spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Special, "Special", "Tag associated with special spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_SpecialAlternative, "SpecialAlternative",
					  "Tag associated with Special Alternate spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Dash, "Dash", "Tag associated with Dash spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Reload, "Reload", "Tag associated with Reloading spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Passive, "Passive", "Tag associated with Passive spells");

	// PLAYER CLASSES //
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Triangle, "PlayerClass.Triangle", "DPS class");
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Circle, "PlayerClass.Circle", "Healer class");
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Square, "PlayerClass.Square", "Tank class");

	// BUFF STATUS //
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_DamageBoost, "Status.Buff.DamageBoost", "Damage boost buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_DamageReduction, "Status.Buff.DamageReduction",
							   "Damage reduction buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_HealBoost, "Status.Buff.HealBoost", "Heal boost buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Speed, "Status.Buff.Speed", "Movement speed buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Shield, "Status.Buff.Shield", "Shield buff");

	// SACRIFICE BEAM //
	CreateAndAssignGameplayTag(GameplayTags.Status_Sacrificed, "Status.Sacrificed",
							   "Sacrificed by the Square's sacrifice beam — incoming damage is redirected.");
	CreateAndAssignGameplayTag(GameplayTags.Status_Square_DetonateReady, "Status.Square.DetonateReady",
							   "The Square has an armed sacrifice detonation: blocks the channel, enables the "
							   "detonate ability, and swaps the ability-bar slot.");

	// ABILITY SPELLS NEEDED IN CODE //
	AddAbilitySpellTag(GameplayTags.Ability_Spell_ShieldBurst, "ShieldBurst", "Ability spell for shield burst");

	// ARENA LOCATION //
	CreateAndAssignGameplayTag(GameplayTags.Arena_FightLocation, "Arena.FightLocation",
							   "Spawn points inside the arena for fight start.");
	CreateAndAssignGameplayTag(GameplayTags.Arena_Entrance, "Arena.Entrance",
							   "Entry point outside arena — dead players teleport here.");
	CreateAndAssignGameplayTag(GameplayTags.Arena_FightCenter, "Arena.FightCenter",
							   "TargetPoint at the center of the fight area.");

	// CAMERA //
	CreateAndAssignGameplayTag(GameplayTags.Camera_Bounds_Intro, "Camera.Bounds.Intro",
							   "Camera bounds used before the fight starts.");
	CreateAndAssignGameplayTag(GameplayTags.Camera_Bounds_Fight, "Camera.Bounds.Fight",
							   "Camera bounds used during the fight.");

	// AI //
	CreateAndAssignGameplayTag(GameplayTags.AI_Boss_AggroEvent, "AI.Boss.AggroEvent",
							   "StateTree event sent when the boss is aggroed.");
	CreateAndAssignGameplayTag(GameplayTags.AI_FiringPoint, "AI.Boss.FiringPoint", "Where the AI can fire from.");
	CreateAndAssignGameplayTag(GameplayTags.AI_Boss_Spawn, "AI.Boss.SpawnLocation",
							   "Tag to determine the spawn of a boss");
	CreateAndAssignGameplayTag(GameplayTags.AI_Dummy_Spawn, "AI.Dummy.SpawnLocation",
							   "Tag to determine the spawn of a Dummy");
}
