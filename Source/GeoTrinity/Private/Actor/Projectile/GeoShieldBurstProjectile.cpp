// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoShieldBurstProjectile.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Tool/Team.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoShieldBurstProjectile::AGeoShieldBurstProjectile()
{
	OverlapAttitude =
		static_cast<int32>(ETeamAttitudeBitflag::Hostile) | static_cast<int32>(ETeamAttitudeBitflag::Friendly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::HandleValidOverlap(AActor* OtherActor)
{
	if (HasAuthority())
	{
		bool const bIsHostile = GeoASLib::IsTeamAttitudeAligned(Payload.Owner, OtherActor,
																static_cast<int32>(ETeamAttitudeBitflag::Hostile));

		if (bIsHostile)
		{
			FVector const Normal = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			float Speed = ProjectileMovement->Velocity.Size();
			FVector const CurrentVelocity = ProjectileMovement->Velocity.GetSafeNormal();
			FVector ReflectedVelocity = CurrentVelocity - 2.f * (FVector::DotProduct(CurrentVelocity, Normal) * Normal);
			ReflectedVelocity.Normalize();
			ReflectedVelocity *= Speed;
			ProjectileMovement->Velocity = ReflectedVelocity;
			ShieldAmount *= EnemyBounceMultiplier;

#if ENABLE_DRAW_DEBUG
			FVector const Origin = GetActorLocation();
			constexpr float DebugLength = 200.f;
			constexpr float ArrowSize = 20.f;
			constexpr float Duration = 3.f;
			DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + CurrentVelocity.GetSafeNormal() * DebugLength,
									  ArrowSize, FColor::Blue, false, Duration, 0, 3.f);
			DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + Normal * DebugLength, ArrowSize, FColor::Green,
									  false, Duration, 0, 3.f);
			DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + ReflectedVelocity.GetSafeNormal() * DebugLength,
									  ArrowSize, FColor::Red, false, Duration, 0, 3.f);
#endif
		}
		else
		{
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(OtherActor);
			if (ensureMsgf(TargetASC, TEXT("AGeoShieldBurstProjectile: no ASC on ally %s"), *OtherActor->GetName()))
			{
				FShieldEffectData ShieldEffect;
				ShieldEffect.ShieldAmount = FScalableFloat(ShieldAmount);
				GeoASLib::ApplySingleEffectData(ShieldEffect, GeoASLib::GetGeoAscFromActor(Payload.Owner), TargetASC,
												Payload.AbilityLevel, Payload.Seed);
			}

			OnProjectileHit(OtherActor);
			EndProjectileLife();
		}
	}
}

void AGeoShieldBurstProjectile::EndProjectileLife()
{
	Super::EndProjectileLife();
}
