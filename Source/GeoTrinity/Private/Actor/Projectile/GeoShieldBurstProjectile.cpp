// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoShieldBurstProjectile.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/Projectile/GeoProjectileFXComponent.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoShieldBurstProjectile::AGeoShieldBurstProjectile()
{
	DefaultParams.OverlapAttitude = TeamAttitudeMask::All;

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
	SphereRadiusToAdd = Sphere->GetScaledSphereRadius() * EnemyBounceAdditiveMultiplier;
	ShieldAmountToAdd = ShieldAmount * EnemyBounceAdditiveMultiplier;

	if (HasAuthority())
	{
		BounceSnapshot = {GetActorLocation(), ProjectileMovement->Velocity, Sphere->GetScaledSphereRadius()};
	}
	// On a client OnRep_BounceSnapshot fired before BeginPlay, whose DefaultParams apply overwrote the radius it set.
	UpdateSizeFX(BounceSnapshot.Radius);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::OnRep_BounceSnapshot()
{
	SetActorLocation(BounceSnapshot.Location);
	ProjectileMovement->Velocity = BounceSnapshot.Velocity;
	ProjectileMovement->UpdateComponentVelocity();
	UpdateSizeFX(BounceSnapshot.Radius);
}

void AGeoShieldBurstProjectile::UpdateSizeFX(float const Radius) const
{
	FXComponent->SetBulletRadius(Radius);
	if (IsValid(BounceSoundSizePitchCurve))
	{
		FXComponent->SetPitchMultiplier(BounceSoundSizePitchCurve->GetFloatValue(Radius));
	}
}

void AGeoShieldBurstProjectile::OnWallBounce(FHitResult const& ImpactResult, FVector const& ImpactVelocity)
{
	if (HasAuthority())
	{
		FVector ReflectedVelocity = ImpactResult.ImpactNormal * ImpactVelocity.Size();
		ReflectedVelocity.Z = 0.f;
		ProjectileMovement->Velocity = ReflectedVelocity;
		ProjectileMovement->UpdateComponentVelocity();
		BounceSnapshot = {GetActorLocation(), ReflectedVelocity, Sphere->GetScaledSphereRadius()};
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::HandleValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent* OwnerASC,
												   UGeoAbilitySystemComponent* TargetASC)
{
	if (GeoASLib::IsTeamAttitudeAligned(GetSourceOwner(), OtherActor, TeamAttitudeMask::HostileOrNeutral))
	{
		FXComponent->PlaySound(BounceSound);
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
			ShieldAmount += ShieldAmountToAdd;
			UpdateSizeFX(Sphere->GetScaledSphereRadius());
			BounceSnapshot = {GetActorLocation(), ReflectedVelocity, Sphere->GetScaledSphereRadius()};
			LastOverlapHostileActor = OtherActor;
			LastOverlapTime = GetWorld()->GetTimeSeconds();
		}
	}
	else
	{
		bEndedOnValidOverlap = true;

		if (GeoLib::IsServer(GetWorld()))
		{
			FShieldEffectData ShieldEffect;
			ShieldEffect.Amount = ShieldAmount;
			GeoASLib::ApplySingleEffectData(ShieldEffect, OwnerASC, TargetASC, Payload.AbilityLevel, Payload.Seed,
											Payload.AbilityTag);
		}

		GeoASLib::NotifyAbilityHit(Payload, OtherActor);

		OnProjectileHit(OtherActor);
		EndProjectileLife();
	}
}
bool AGeoShieldBurstProjectile::IsValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent*& OutOwnerASC,
											   UGeoAbilitySystemComponent*& OutTargetASC)
{
	constexpr float TimeThresholdBetweenSameHostileOverlap = 0.5f;
	bool const bRepeatHostileOverlap = LastOverlapHostileActor.IsValid() && LastOverlapHostileActor == OtherActor
		&& GetWorld()->GetTimeSeconds() - LastOverlapTime < TimeThresholdBetweenSameHostileOverlap;
	bool const bFriendlyDeployable = OtherActor->IsA(AGeoDeployableBase::StaticClass())
		&& GeoASLib::IsTeamAttitudeAligned(GetSourceOwner(), OtherActor, TeamAttitudeMask::Friendly);
	if (bFriendlyDeployable || bRepeatHostileOverlap)
	{
		return false;
	}

	return Super::IsValidOverlap(OtherActor, OutOwnerASC, OutTargetASC);
}
