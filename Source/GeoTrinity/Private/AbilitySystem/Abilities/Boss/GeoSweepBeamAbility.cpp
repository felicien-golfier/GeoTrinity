// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSweepBeamAbility.h"

#include "Actor/GeoHexArena.h"

float UGeoSweepBeamAbility::GetFireYaw(AActor const* Instigator, int const Seed) const
{
	AGeoHexArena const* const Arena = AGeoHexArena::GetArenaOfBoss(Instigator);
	if (!ensureMsgf(Arena, TEXT("UGeoSweepBeamAbility: %s is not a hex arena boss"), *GetNameSafe(Instigator)))
	{
		return Super::GetFireYaw(Instigator, Seed);
	}

	FVector ToCenter = Arena->GetActorLocation() - Instigator->GetActorLocation();
	ToCenter.Z = 0.f;

	ToCenter = ToCenter.RotateAngleAxis(Angle * (Seed % 2 == 0 ? 1.f : -1.f), FVector(0.f, 0.f, 1.f));
	return ToCenter.Rotation().Yaw;
}
