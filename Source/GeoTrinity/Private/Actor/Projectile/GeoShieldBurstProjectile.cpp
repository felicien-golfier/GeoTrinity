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
#include "NiagaraComponent.h"
#include "Settings/GameDataSettings.h"
#include "TimerManager.h"
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
void AGeoShieldBurstProjectile::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GeoLib::IsServer(GetWorld()) && !bIsEnding
		&& GetWorld()->GetTimeSeconds() >= BounceSnapshot.ServerTime + ResyncInterval)
	{
		BounceSnapshot = CaptureSnapshot(BounceSnapshot.EnemyBounceCount);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::InitProjectileLife()
{
	Super::InitProjectileLife();
	ProjectileMovement->OnProjectileBounce.AddUniqueDynamic(this, &ThisClass::OnWallBounce);
	SphereRadiusToAdd = Sphere->GetScaledSphereRadius() * EnemyBounceAdditiveMultiplier;
	ShieldAmountToAdd = ShieldAmount * EnemyBounceAdditiveMultiplier;

	FTimerHandle BlinkStartHandle;
	GetWorldTimerManager().SetTimer(BlinkStartHandle,
									FTimerDelegate::CreateWeakLambda(this,
																	 [this]()
																	 {
																		 if (!bIsEnding)
																		 {
																			 FXComponent->StartBlinking(BlinkDuration);
																		 }
																	 }),
									LifeSpanInSec - BlinkDuration, false);

	if (GeoLib::IsServer(GetWorld()))
	{
		BounceSnapshot = CaptureSnapshot(0);
		UpdateSizeFX(BounceSnapshot.Radius);
	}
	else
	{
		ApplyBounceSnapshot();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::OnRep_BounceSnapshot()
{
	bool const bOlderThanPredictedBounce = BounceSnapshot.EnemyBounceCount < ClientSnapshot.EnemyBounceCount
		&& BounceSnapshot.ServerTime < ClientSnapshot.ServerTime;
	if (bOlderThanPredictedBounce)
	{
		return;
	}

	if (BounceSnapshot.EnemyBounceCount > ClientSnapshot.EnemyBounceCount)
	{
		FXComponent->PlaySound(BounceSound);
	}

	ClientSnapshot = BounceSnapshot;
	// Before BeginPlay, InitProjectileLife applies it once the projectile's life has started.
	if (HasActorBegunPlay() && !bIsEnding)
	{
		ApplyBounceSnapshot();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::ApplyBounceSnapshot()
{
	FVector const DrawnLocation = BulletVFX->GetComponentLocation();

	// Velocity first: an overlap the teleport starts must see the velocity the burst leaves with.
	ProjectileMovement->Velocity = BounceSnapshot.Velocity;
	ProjectileMovement->UpdateComponentVelocity();
	SetActorLocation(BounceSnapshot.Location);
	Sphere->SetSphereRadius(BounceSnapshot.Radius);
	UpdateSizeFX(BounceSnapshot.Radius);

	float const TransitTime = FMath::Clamp(GeoLib::GetServerTime(GetWorld(), true) - BounceSnapshot.ServerTime, 0.f,
										   GetDefault<UGameDataSettings>()->MaxLatencyCompensation);
	ProjectileMovement->TickComponent(TransitTime, LEVELTICK_All, nullptr);

	FXComponent->SetVisualLaunchLocation(DrawnLocation);
}

// ---------------------------------------------------------------------------------------------------------------------
FShieldBounceSnapshot AGeoShieldBurstProjectile::CaptureSnapshot(int32 const EnemyBounceCount) const
{
	return {GetActorLocation(), ProjectileMovement->Velocity, Sphere->GetScaledSphereRadius(), EnemyBounceCount,
			GeoLib::GetServerTime(GetWorld(), true)};
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::UpdateSizeFX(float const Radius) const
{
	FXComponent->SetBulletRadius(Radius);
	if (IsValid(BounceSoundSizePitchCurve))
	{
		FXComponent->SetPitchMultiplier(BounceSoundSizePitchCurve->GetFloatValue(Radius));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::OnWallBounce(FHitResult const& ImpactResult, FVector const& ImpactVelocity)
{
	FVector ReflectedVelocity = ImpactResult.ImpactNormal * ImpactVelocity.Size();
	ReflectedVelocity.Z = 0.f;
	ProjectileMovement->Velocity = ReflectedVelocity;
	ProjectileMovement->UpdateComponentVelocity();

	if (GeoLib::IsServer(GetWorld()))
	{
		BounceSnapshot = CaptureSnapshot(BounceSnapshot.EnemyBounceCount);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoShieldBurstProjectile::HandleValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent* OwnerASC,
												   UGeoAbilitySystemComponent* TargetASC)
{
	bool const bIsServer = GeoLib::IsServer(GetWorld());
	if (GeoASLib::IsTeamAttitudeAligned(GetSourceOwner(), OtherActor, TeamAttitudeMask::HostileOrNeutral))
	{
		FXComponent->PlaySound(BounceSound);
		FVector const Normal = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		float const Speed = ProjectileMovement->Velocity.Size();
		FVector const CurrentVelocity = ProjectileMovement->Velocity.GetSafeNormal2D();
		FVector ReflectedVelocity = CurrentVelocity - 2.f * (FVector::DotProduct(CurrentVelocity, Normal) * Normal);
		ReflectedVelocity.Normalize();
		ReflectedVelocity *= Speed;
		ReflectedVelocity.Z = 0.f;
		ProjectileMovement->Velocity = ReflectedVelocity;
		ProjectileMovement->UpdateComponentVelocity();
		LastOverlapHostileActor = OtherActor;
		LastOverlapTime = GetWorld()->GetTimeSeconds();

		if (bIsServer)
		{
			Sphere->SetSphereRadius(Sphere->GetScaledSphereRadius() + SphereRadiusToAdd);
			ShieldAmount += ShieldAmountToAdd;
			UpdateSizeFX(Sphere->GetScaledSphereRadius());
			BounceSnapshot = CaptureSnapshot(BounceSnapshot.EnemyBounceCount + 1);
		}
		else
		{
			ClientSnapshot = CaptureSnapshot(ClientSnapshot.EnemyBounceCount + 1);
		}
	}
	else
	{
		OnProjectileConfirmedOverlap(OtherActor);

		if (bIsServer)
		{
			bEndedOnValidOverlap = true;
			FShieldEffectData ShieldEffect;
			ShieldEffect.Amount = ShieldAmount;
			GeoASLib::ApplySingleEffectData(ShieldEffect, OwnerASC, TargetASC, Payload.AbilityLevel, Payload.Seed,
											Payload.AbilityTag);
			GeoASLib::NotifyAbilityHit(Payload, OtherActor);
			EndProjectileLife();
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoShieldBurstProjectile::IsValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent*& OutOwnerASC,
											   UGeoAbilitySystemComponent*& OutTargetASC)
{
	constexpr float TimeThresholdBetweenSameHostileOverlap = 0.5f;
	bool const bRepeatHostileOverlap = LastOverlapHostileActor.IsValid() && LastOverlapHostileActor == OtherActor
		&& GetWorld()->GetTimeSeconds() - LastOverlapTime < TimeThresholdBetweenSameHostileOverlap;
	bool const bFriendlyDeployable = OtherActor->IsA(AGeoDeployableBase::StaticClass())
		&& GeoASLib::IsTeamAttitudeAligned(GetSourceOwner(), OtherActor, TeamAttitudeMask::Friendly);
	bool const bMovingAwayFromHostile =
		FVector::DotProduct(ProjectileMovement->Velocity, OtherActor->GetActorLocation() - GetActorLocation()) <= 0.f
		&& GeoASLib::IsTeamAttitudeAligned(GetSourceOwner(), OtherActor, TeamAttitudeMask::HostileOrNeutral);
	if (bFriendlyDeployable || bRepeatHostileOverlap || bMovingAwayFromHostile)
	{
		return false;
	}

	return Super::IsValidOverlap(OtherActor, OutOwnerASC, OutTargetASC);
}
