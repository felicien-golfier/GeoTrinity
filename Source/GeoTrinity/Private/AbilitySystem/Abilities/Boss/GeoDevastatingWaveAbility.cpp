// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoDevastatingWaveAbility.h"

#include "Tool/UGeoGameplayLibrary.h"

FVector2D UGeoDevastatingWaveAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* /*SourceASC*/,
													  int /*Seed*/) const
{
	TArray<AActor*> const TargetPoints = GeoLib::GetTargetPoints(Instigator, TeleportLocationTag);
	if (!ensureMsgf(!TargetPoints.IsEmpty(),
					TEXT("UGeoDevastatingWaveAbility: no AGeoTargetPoint found for TeleportLocationTag %s"),
					*TeleportLocationTag.ToString()))
	{
		return FVector2D::ZeroVector;
	}

	return FVector2D(TargetPoints[0]->GetActorLocation());
}
