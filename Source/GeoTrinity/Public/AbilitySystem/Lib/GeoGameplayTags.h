#pragma once
#include "GameplayTagContainer.h"


namespace RootTagNames
{
	FString const AbilityTag{"Ability"};
	FString const InputTag{"InputTag"};
	FString const SpellSubTag{"Spell"};
	FString const AbilityIDTag{AbilityTag + "." + SpellSubTag};
} // namespace RootTagNames

/**
 * GameplayTags
 *
 * Singleton containing native gameplay tags
 */
struct FGeoGameplayTags
{
	static FGeoGameplayTags const& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();

	FGameplayTag Gameplay_Damage;
	FGameplayTag Gameplay_Heal;
	FGameplayTag Gameplay_Shield;
	FGameplayTag Gameplay_Drain;
	FGameplayTag Gameplay_DurationMagnitude;
	FGameplayTag Gameplay_GenericMagnitude;

	// Input
	FGameplayTag InputTag_Basic;
	FGameplayTag InputTag_Special;
	FGameplayTag InputTag_SpecialAlternative;
	FGameplayTag InputTag_Reload;
	FGameplayTag InputTag_Dash;

	// Ability types
	FGameplayTag Ability_Type_Basic;
	FGameplayTag Ability_Type_Special;
	FGameplayTag Ability_Type_SpecialAlternative;
	FGameplayTag Ability_Type_Dash;
	FGameplayTag Ability_Type_Reload;

	// Player classes
	FGameplayTag PlayerClass_Triangle;
	FGameplayTag PlayerClass_Circle;
	FGameplayTag PlayerClass_Square;

	// Ability states
	FGameplayTag Status_Reloading;

	// Buff status
	FGameplayTag Status_Buff_DamageBoost;
	FGameplayTag Status_Buff_DamageReduction;
	FGameplayTag Status_Buff_HealBoost;
	FGameplayTag Status_Buff_Speed;
	FGameplayTag Status_Buff_Shield;

private:
	static FGeoGameplayTags GameplayTags;
};
