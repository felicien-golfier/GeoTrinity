// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GeoAbilitySystemGlobals.h"

FGameplayEffectContext* UGeoAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FGeoGameplayEffectContext();
}
