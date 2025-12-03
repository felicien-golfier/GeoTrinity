// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GameplayTagContainer.h"


namespace RootTagNames
{
	const FString AbilityTag{"Ability"};
	const FString InputTag{"InputTag"};
	const FString SpellSubTag{"Spell"};
	const FString AbilityIDTag{AbilityTag + "." + SpellSubTag};
}

/**
 * GameplayTags
 *
 * Singleton containing native gameplay tags
 */
struct FGeoGameplayTags
{
	static FGeoGameplayTags const& Get() {return GameplayTags;}
	static void InitializeNativeGameplayTags();
   
	FGameplayTag Gameplay_Damage;
	
	FGameplayTag InputTag_BasicSpell;
	FGameplayTag InputTag_SpecialSpell;
	
	FGameplayTag Ability_Type_Basic;
	FGameplayTag Ability_Type_Special;

private:
	static FGeoGameplayTags GameplayTags;
};