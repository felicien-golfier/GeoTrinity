// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoDevastatingWaveAbility.h"

FVector2D UGeoDevastatingWaveAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
													  int Seed) const
{
	return TeleportLocation;
}
