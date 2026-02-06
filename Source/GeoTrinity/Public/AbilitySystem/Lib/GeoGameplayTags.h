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

	// Input
	FGameplayTag InputTag_BasicSpell;
	FGameplayTag InputTag_SpecialSpell;
	FGameplayTag InputTag_Reload;
	FGameplayTag InputTag_Deploy;
	FGameplayTag InputTag_AltSpecial;
	FGameplayTag InputTag_Dash;

	// Ability types
	FGameplayTag Ability_Type_Basic;
	FGameplayTag Ability_Type_Special;
	FGameplayTag Ability_Type_Dash;

	// Player classes
	FGameplayTag PlayerClass_Triangle;
	FGameplayTag PlayerClass_Circle;
	FGameplayTag PlayerClass_Square;

	// Buff status
	FGameplayTag Status_Buff_DamageBoost;
	FGameplayTag Status_Buff_Tankiness;
	FGameplayTag Status_Buff_HealBoost;
	FGameplayTag Status_Buff_Speed;
	FGameplayTag Status_Buff_Heal;
	FGameplayTag Status_Buff_Shield;

private:
	static FGeoGameplayTags GameplayTags;
};
