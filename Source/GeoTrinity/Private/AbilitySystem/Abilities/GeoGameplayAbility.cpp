// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GeoGameplayAbility.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoGameplayAbility::GetAbilityTag() const
{
	return UGeoAbilitySystemLibrary::GetAbilityTagFromAbility(*this);
}