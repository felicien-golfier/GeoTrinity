// Fill out your copyright notice in the Description page of Project Settings.

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

	FGameplayTag InputTag_BasicSpell;
	FGameplayTag InputTag_SpecialSpell;

	FGameplayTag Ability_Type_Basic;
	FGameplayTag Ability_Type_Special;

private:
	static FGeoGameplayTags GameplayTags;
};
