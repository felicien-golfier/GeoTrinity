// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Projectile/GeoProjectile.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoProjectile::AGeoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);

	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	// TODO: we will need to see whether we create a specific channel for "characters", in the case we use a different
	// shape for collision (instead of the Capsule)
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetCollisionObjectType(ECC_Projectile);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::LifeSpanExpired()
{
	if (LifeSpanInSec != 0)
	{
		EndProjectileLife();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float elapsedDistanceSqr = FVector::DistSquared(GetActorLocation(), InitialPosition);
	if (elapsedDistanceSqr >= DistanceSpanSqr)
	{
		EndProjectileLife();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(true);

	DistanceSpanSqr = FMath::Square(DistanceSpan);
	SetLifeSpan(LifeSpanInSec);

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);

	LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, GetRootComponent(), NAME_None,
		FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);

	InitialPosition = GetActorLocation();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::Destroyed()
{
	if (!bHit && !HasAuthority())
	{
		// In case destroy() is replicated before OnSphereOverlap()
		PlayImpactFx();
	}
	Super::Destroyed();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::ApplyEffectToTarget(AActor* OtherActor)
{
	DamageEffectParams.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	DamageEffectParams.DeathImpulseVector = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;
	DamageEffectParams.DeathImpulseVector.Z = 0;   // Parallel to the ground

	if (DamageEffectParams.KnockbackChancePercent
		&& DamageEffectParams.KnockbackChancePercent >= FMath::RandRange(1, 100))
	{
		// Override pitch
		const FVector knockbackDirection = GetActorForwardVector().RotateAngleAxis(-45.f, GetActorRightVector());
		DamageEffectParams.KnockbackVector = knockbackDirection * DamageEffectParams.KnockbackMagnitude;
	}

	UGeoAbilitySystemLibrary::ApplyEffectFromDamageParams(DamageEffectParams);
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoProjectile::IsValidOverlap(const AActor* OtherActor)
{
	if (!IsValid(DamageEffectParams.SourceASC))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("A projectile was launched with an invalid Source ASC, this should never happen"));
		return false;
	}

	const AActor* pSourceAvatarActor = DamageEffectParams.SourceASC->GetAvatarActor();

	// Don't apply on self
	if (!pSourceAvatarActor || (pSourceAvatarActor == OtherActor))
	{
		return false;
	}

	// TODO: add a way to enable/remove friendly fire (setup the team interface stuff ?)

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::StopLoopingSound() const
{
	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidOverlap(OtherActor))
	{
		return;
	}

	// If multiple overlap, don't play sound each time
	if (!bHit)
	{
		PlayImpactFx();
	}

	bHit = true;
	if (HasAuthority())
	{
		ApplyEffectToTarget(OtherActor);
		EndProjectileLife();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PlayImpactFx() const
{
	if (!IsValid(this))
	{
		return;
	}

	StopLoopingSound();

	const FVector actorLocation = GetActorLocation();
	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, actorLocation, FRotator::ZeroRotator, 0.2f);
	}
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, actorLocation);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::EndProjectileLife()
{
	Destroy();
}
