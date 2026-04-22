#pragma once
#include "GameplayTagContainer.h"


namespace RootTagNames
{
	FString const AbilityTag{"Ability"};
	FString const InputTag{"InputTag"};
	FString const SpellSubTag{"Spell"};
	FString const AbilitySpellTag{AbilityTag + "." + SpellSubTag};
	FString const AbilityTypeTag{AbilityTag + "." + "Type"};
} // namespace RootTagNames

/**
 * GameplayTags
 *
 * Singleton containing native gameplay tags
 */
struct FGeoGameplayTags
{
	/** Returns the singleton instance of all native gameplay tags. */
	static FGeoGameplayTags const& Get() { return GameplayTags; }
	/** Registers all native tags with the GameplayTagsManager. Called from UGeoAssetManager::StartInitialLoading. */
	static void InitializeNativeGameplayTags();
	static bool AreNativeTagsInitialized() { return bNativeTagsInitialized; }

	FGameplayTag Gameplay_Damage;
	FGameplayTag Gameplay_Heal;
	FGameplayTag Gameplay_Shield;
	FGameplayTag Gameplay_DurationMagnitude;
	FGameplayTag Gameplay_GenericMagnitude;

	// Input
	FGameplayTag InputTag_Basic;
	FGameplayTag InputTag_Special;
	FGameplayTag InputTag_SpecialAlternative;
	FGameplayTag InputTag_Reload;
	FGameplayTag InputTag_Dash;

	// Ability types
	FGameplayTag Ability_Type;
	FGameplayTag Ability_Type_Basic;
	FGameplayTag Ability_Type_Special;
	FGameplayTag Ability_Type_SpecialAlternative;
	FGameplayTag Ability_Type_Dash;
	FGameplayTag Ability_Type_Reload;
	FGameplayTag Ability_Type_Passive;

	// Player classes
	FGameplayTag PlayerClass_Triangle;
	FGameplayTag PlayerClass_Circle;
	FGameplayTag PlayerClass_Square;

	// Buff status
	FGameplayTag Status_Buff_DamageBoost;
	FGameplayTag Status_Buff_DamageReduction;
	FGameplayTag Status_Buff_HealBoost;
	FGameplayTag Status_Buff_Speed;
	FGameplayTag Status_Buff_Shield;

	// Ability spells needed in code
	FGameplayTag Ability_Spell_ShieldBurst;


private:
	static FGeoGameplayTags GameplayTags;
	static bool bNativeTagsInitialized;
};
