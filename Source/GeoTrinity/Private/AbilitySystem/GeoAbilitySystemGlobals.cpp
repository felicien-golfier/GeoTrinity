// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/GeoAbilitySystemGlobals.h"

FGameplayEffectContext* UGeoAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FGeoGameplayEffectContext();
}
