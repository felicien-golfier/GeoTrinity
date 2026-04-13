// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoShieldBurstProjectile.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Tool/Team.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoShieldBurstProjectile::AGeoShieldBurstProjectile()
{
	OverlapAttitude = static_cast<int32>(ETeamAttitudeBitflag::Hostile) | static_cast<int32>(ETeamAttitudeBitflag::Friendly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::HandleValidOverlap(AActor* OtherActor)
{
	bool const bIsHostile =
		GeoASLib::IsTeamAttitudeAligned(Payload.Owner, OtherActor, static_cast<int32>(ETeamAttitudeBitflag::Hostile));

	if (bIsHostile)
	{
		FVector const Normal = (GetActorLocation() - OtherActor->GetActorLocation()).GetSafeNormal();
		FVector const CurrentVelocity = ProjectileMovement->Velocity;
		ProjectileMovement->Velocity = CurrentVelocity - 2.f * FVector::DotProduct(CurrentVelocity, Normal) * Normal;
		ShieldAmount *= EnemyBounceMultiplier;
		return;
	}

	if (HasAuthority())
	{
		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(OtherActor);
		if (ensureMsgf(TargetASC, TEXT("AGeoShieldBurstProjectile: no ASC on ally %s"), *OtherActor->GetName()))
		{
			FShieldEffectData ShieldEffect;
			ShieldEffect.ShieldAmount = FScalableFloat(ShieldAmount);
			GeoASLib::ApplySingleEffectData(ShieldEffect, GeoASLib::GetGeoAscFromActor(Payload.Owner), TargetASC,
											Payload.AbilityLevel, Payload.Seed);
		}
	}

	OnProjectileHit(OtherActor);
	EndProjectileLife();
}
