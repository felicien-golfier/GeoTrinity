// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemGlobals.h"
#include "CoreMinimal.h"
#include "GeoAscTypes.h"

#include "GeoAbilitySystemGlobals.generated.h"

/**
 * Direved to allow the use of a custom Gameplay effect context class
 */
UCLASS()
class GEOTRINITY_API UGeoAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
