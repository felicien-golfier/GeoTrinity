// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "GeoAscTypes.h"

#include "GeoAbilitySystemGlobals.generated.h"

/**
 * Direved to allow the use of a custom Gameplay effect context class
 */
UCLASS()
class GEOTRINITY_API UGeoAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	virtual  FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
