// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoShieldBurstProjectile.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Wall/GeoWall.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoShieldBurstProjectile::AGeoShieldBurstProjectile()
{
	OverlapAttitude = TeamAttitudeMask::All;

	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 1.0f;
	ProjectileMovement->Friction = 0.0f;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoShieldBurstProjectile, BounceSnapshot);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::InitProjectileLife()
{
	Super::InitProjectileLife();
	ProjectileMovement->OnProjectileBounce.AddUniqueDynamic(this, &ThisClass::OnWallBounce);
	SphereRadiusToAdd = Sphere->GetScaledSphereRadius() * EnemyBounceMultiplier - Sphere->GetScaledSphereRadius();
	ShieldAmountToAdd = ShieldAmount.Value * EnemyBounceMultiplier - ShieldAmount.Value;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::OnRep_BounceSnapshot()
{
	SetActorLocation(BounceSnapshot.Location);
	ProjectileMovement->Velocity = BounceSnapshot.Velocity;
	ProjectileMovement->UpdateComponentVelocity();
	UpdateVisualRadius(BounceSnapshot.Radius);
}

void AGeoShieldBurstProjectile::UpdateVisualRadius(float Radius) const
{
	if (BulletVFX)
	{
		BulletVFX->SetVariableFloat(FName("User.Bullet_Radius"), Radius);
	}
}

float AGeoShieldBurstProjectile::GetPitch(FGeoSoundEntry const& Entry) const
{
	float Pitch = Super::GetPitch(Entry);
	if (IsValid(BounceSoundSizePitchCurve))
	{
		Pitch *= BounceSoundSizePitchCurve->GetFloatValue(Sphere->GetScaledSphereRadius());
	}
	return Pitch;
}

void AGeoShieldBurstProjectile::OnWallBounce(FHitResult const& ImpactResult, FVector const& ImpactVelocity)
{
	if (HasAuthority())
	{
		BounceSnapshot = {GetActorLocation(), ImpactVelocity, Sphere->GetScaledSphereRadius()};
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::HandleValidOverlap(AActor* OtherActor)
{
	AActor* const CurrentOwner = IsValid(Payload.Owner) ? Payload.Owner : GetOwner();
	if (GeoASLib::IsTeamAttitudeAligned(CurrentOwner, OtherActor, TeamAttitudeMask::HostileOrNeutral))
	{
		PlaySoundOneShot(BounceSound);
		if (GeoLib::IsServer(GetWorld()))
		{
			FVector const Normal = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			float const Speed = ProjectileMovement->Velocity.Size();
			FVector const CurrentVelocity = ProjectileMovement->Velocity.GetSafeNormal2D();
			FVector ReflectedVelocity = CurrentVelocity - 2.f * (FVector::DotProduct(CurrentVelocity, Normal) * Normal);
			ReflectedVelocity.Normalize();
			ReflectedVelocity *= Speed;
			ReflectedVelocity.Z = 0.f;
			ProjectileMovement->Velocity = ReflectedVelocity;
			ProjectileMovement->UpdateComponentVelocity();
			Sphere->SetSphereRadius(Sphere->GetScaledSphereRadius() + SphereRadiusToAdd);
			ShieldAmount.Value += ShieldAmountToAdd;
			UpdateVisualRadius(Sphere->GetScaledSphereRadius());
			BounceSnapshot = {GetActorLocation(), ReflectedVelocity, Sphere->GetScaledSphereRadius()};
			LastOverlapHostileActor = OtherActor;
			LastOverlapTime = GetWorld()->GetTimeSeconds();
		}
	}
	else
	{
		EndSoundType = EProjectileSoundType::ValidOverlapEnd;
		if (GeoLib::IsServer(GetWorld()))
		{
			// TODO: as we don;t call super, we should ensure few things before the ensure
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(OtherActor);
			if (ensureMsgf(TargetASC, TEXT("AGeoShieldBurstProjectile: no ASC on ally %s"), *OtherActor->GetName()))
			{
				FShieldEffectData ShieldEffect;
				ShieldEffect.ShieldAmount = ShieldAmount;
				GeoASLib::ApplySingleEffectData(ShieldEffect, GeoASLib::GetGeoAscFromActor(Payload.Owner), TargetASC,
												Payload.AbilityLevel, Payload.Seed, Payload.AbilityTag);
			}
			OnProjectileHit(OtherActor);
			EndProjectileLife();
		}
	}
}
bool AGeoShieldBurstProjectile::IsValidOverlap(AActor* OtherActor)
{
	if (OtherActor->IsA(AGeoWall::StaticClass()))
	{
		return false;
	}

	constexpr float TimeThresholdBetweenSameHostileOverlap = 0.5f;
	if (LastOverlapHostileActor.IsValid() && LastOverlapHostileActor == OtherActor
		&& GetWorld()->GetTimeSeconds() - LastOverlapTime < TimeThresholdBetweenSameHostileOverlap)
	{
		return false;
	}

	return Super::IsValidOverlap(OtherActor);
}
