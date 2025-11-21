// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Projectile/GeoProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/AudioComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


// ---------------------------------------------------------------------------------------------------------------------
AGeoProjectile::AGeoProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);
	
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	// TODO: we will need to see whether we create a specific channel for "characters", in the case we use a different shape for collision (instead of the Capsule)
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetCollisionObjectType(ECC_Projectile);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpanInSec);
	
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);

	LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, GetRootComponent(), NAME_None,
		FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
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
	DamageEffectParams.DeathImpulseVector.Z = 0;	// Parallel to the ground

	if (DamageEffectParams.KnockbackChancePercent &&
		DamageEffectParams.KnockbackChancePercent >= FMath::RandRange(1, 100))
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
		UE_LOG(LogGeoTrinity, Error, TEXT("A projectile was launched with an invalid Source ASC, this should never happen"));
		return false;
	}
	
	AActor const* pSourceAvatarActor = DamageEffectParams.SourceASC->GetAvatarActor();
	
	// Don't apply on self
	if (!pSourceAvatarActor || (pSourceAvatarActor == OtherActor))
		return false;
	
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
	UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult)
{
	if (!IsValidOverlap(OtherActor))
		return;
	
	// If multiple overlap, don't play sound each time
	if (!bHit)
	{
		PlayImpactFx();
	}

	bHit = true;
	if (HasAuthority())
	{
		ApplyEffectToTarget(OtherActor);
		Destroy();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PlayImpactFx() const
{
	if (!IsValid(this))
		return;

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


