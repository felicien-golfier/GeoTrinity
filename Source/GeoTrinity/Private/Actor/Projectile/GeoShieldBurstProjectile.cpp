// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoShieldBurstProjectile.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Tool/Team.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoShieldBurstProjectile::AGeoShieldBurstProjectile()
{
	OverlapAttitude =
		static_cast<int32>(ETeamAttitudeBitflag::Hostile) | static_cast<int32>(ETeamAttitudeBitflag::Friendly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoShieldBurstProjectile, BounceSnapshot);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::OnRep_BounceSnapshot()
{
	SetActorLocation(BounceSnapshot.Location);
	ProjectileMovement->Velocity = BounceSnapshot.Velocity;
	ProjectileMovement->UpdateComponentVelocity();
	UNiagaraComponent* Niagara = GetComponentByClass<UNiagaraComponent>();
	if (!ensureMsgf(Niagara, TEXT("AGeoShieldBurstProjectile: no Niagara on %s"), *GetName()))
	{
		return;
	}
	Niagara->SetVariableFloat(FName("User.Bullet_Radius"), BounceSnapshot.Radius);
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
			FVector const Normal = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			float Speed = ProjectileMovement->Velocity.Size();
			FVector const CurrentVelocity = ProjectileMovement->Velocity.GetSafeNormal2D();
			FVector ReflectedVelocity = CurrentVelocity - 2.f * (FVector::DotProduct(CurrentVelocity, Normal) * Normal);
			ReflectedVelocity.Normalize();
			ReflectedVelocity *= Speed;
			ProjectileMovement->Velocity = ReflectedVelocity;
			ShieldAmount *= EnemyBounceMultiplier;
			Sphere->SetSphereRadius(Sphere->GetScaledSphereRadius() * EnemyBounceMultiplier);
			BounceSnapshot = {GetActorLocation(), ReflectedVelocity, Sphere->GetScaledSphereRadius()};
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
