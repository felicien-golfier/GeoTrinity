// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSweepBeamAbility.h"

#include "Actor/GeoHexArena.h"

float UGeoSweepBeamAbility::GetFireYaw(AActor const* Instigator) const
{
	AGeoHexArena const* const Arena = AGeoHexArena::GetArenaOfBoss(Instigator);
	if (!ensureMsgf(Arena, TEXT("UGeoSweepBeamAbility: %s is not a hex arena boss"), *GetNameSafe(Instigator)))
	{
		return Super::GetFireYaw(Instigator);
	}

	FVector ToCenter = Arena->GetActorLocation() - Instigator->GetActorLocation();
	ToCenter.Z = 0.f;

	ToCenter = ToCenter.RotateAngleAxis(Angle * (StoredPayload.Seed % 2 == 0 ? 1 : -1), FVector(0.f, 0.f, 1.f));
	return ToCenter.Rotation().Yaw;
}
