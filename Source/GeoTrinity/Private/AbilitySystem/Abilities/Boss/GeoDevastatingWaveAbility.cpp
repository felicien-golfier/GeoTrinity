// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoDevastatingWaveAbility.h"

#include "Actor/GeoArena.h"
#include "Tool/UGeoGameplayLibrary.h"

FVector2D UGeoDevastatingWaveAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* /*SourceASC*/,
													  int /*Seed*/) const
{
	AGeoArena const* const Arena = AGeoArena::GetArenaOfBoss(Instigator);
	if (!ensureMsgf(Arena, TEXT("UGeoDevastatingWaveAbility: %s was not spawned by an arena"),
					*GetNameSafe(Instigator)))
	{
		return FVector2D::ZeroVector;
	}

	TArray<AActor*> const TargetPoints = GeoLib::GetTargetPoints(Instigator, TeleportLocationTag, Arena->ArenaTag);
	if (!ensureMsgf(!TargetPoints.IsEmpty(),
					TEXT("UGeoDevastatingWaveAbility: no AGeoTargetPoint tagged %s in arena %s"),
					*TeleportLocationTag.ToString(), *Arena->ArenaTag.ToString()))
	{
		return FVector2D::ZeroVector;
	}

	return FVector2D(TargetPoints[0]->GetActorLocation());
}
