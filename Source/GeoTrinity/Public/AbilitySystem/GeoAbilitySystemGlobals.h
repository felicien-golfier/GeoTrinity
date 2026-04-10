// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemGlobals.h"
#include "CoreMinimal.h"
#include "GeoAscTypes.h"

#include "GeoAbilitySystemGlobals.generated.h"

/**
 * Derived from UAbilitySystemGlobals to inject the custom FGeoGameplayEffectContext.
 * Registered in DefaultGame.ini so GAS uses this class when allocating effect contexts.
 */
UCLASS()
class GEOTRINITY_API UGeoAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	/** Allocates a FGeoGameplayEffectContext instead of the default FGameplayEffectContext. */
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
