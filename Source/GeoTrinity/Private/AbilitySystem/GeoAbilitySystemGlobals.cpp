// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/GeoAbilitySystemGlobals.h"

FGameplayEffectContext* UGeoAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FGeoGameplayEffectContext();
}

void UGeoAbilitySystemGlobals::InitGameplayCueParameters(FGameplayCueParameters& CueParameters,
														 FGameplayEffectContextHandle const& EffectContext)
{
	// Super::InitGameplayCueParameters(CueParameters, EffectContext); We don't need the full effect context !
	CueParameters.Instigator = EffectContext.GetInstigator();
	CueParameters.EffectCauser = EffectContext.GetEffectCauser();
	if (FHitResult const* HitResult = EffectContext.GetHitResult())
	{
		CueParameters.Location = HitResult->ImpactPoint;
		CueParameters.Normal = HitResult->ImpactNormal;
	}
}
