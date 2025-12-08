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
#include "System/GeoActorPoolingSubsystem.h"
#include "Tool/GameplayLibrary.h"

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

	LoopingSoundComponent = CreateDefaultSubobject<UAudioComponent>("LoopingSoundComponent");
	LoopingSoundComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::LifeSpanExpired()
{
	if (LifeSpanInSec != 0 && !bIsEnding)
	{
		bIsEnding = true;
		EndProjectileLife();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsEnding)
	{
		return;
	}

	float elapsedDistanceSqr = FVector::DistSquared(GetActorLocation(), InitialPosition);
	if (elapsedDistanceSqr >= DistanceSpanSqr)
	{
		bIsEnding = true;
		EndProjectileLife();
	}

	UE_VLOG_SPHERE(this, LogGeoTrinity, Verbose, GetActorLocation(), GetSimpleCollisionRadius(),
		GameplayLibrary::GetColorForObject(GetOuter()), TEXT("Projectile tick of %s"), *GetName());
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
	// Do no execute overlap logic on client !
	if (!HasAuthority())
	{
		return false;
	}

	if (bIsEnding)
	{
		return false;
	}

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

	const IGenericTeamAgentInterface* TeamInterface = nullptr;
	if (!GameplayLibrary::GetTeamInterface(OtherActor, TeamInterface))
	{
		return false;
	}

	return TeamInterface->GetGenericTeamId().GetId() != FGenericTeamId::NoTeam
	    && (TeamInterface->GetGenericTeamId().GetId() & ApplyEffectToTeamOnOverlap) != 0x00;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidOverlap(OtherActor))
	{
		return;
	}

	bIsEnding = true;

	PlayImpactFx();

	if (HasAuthority())
	{
		ApplyEffectToTarget(OtherActor);
	}

	EndProjectileLife();
}

void AGeoProjectile::OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)

{
	if (bIsEnding)
	{
		return;
	}
	bIsEnding = true;
	EndProjectileLife();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PlayImpactFx() const
{
	if (!IsValid(this))
	{
		return;
	}

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
	PlayImpactFx();

	// TODO: Call Release after FX are done !

	UGeoActorPoolingSubsystem* Pool = GetWorld()->GetSubsystem<UGeoActorPoolingSubsystem>();
	checkf(Pool, TEXT("GeoActorPoolingSubsystem is invalid!"));
	Pool->ReleaseActor(this);
}

void AGeoProjectile::InitProjectileMovementComponent()
{
	// Clear any previous movement state
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetUpdatedComponent(Sphere);

	if (ProjectileMovement->InitialSpeed > 0.f)
	{
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
		if (ProjectileMovement->bRotationFollowsVelocity)
		{
			SetActorRotation(ProjectileMovement->Velocity.Rotation());
		}
		ProjectileMovement->UpdateComponentVelocity();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::Init()
{
	if (!HasAuthority())
	{
		LoopingSoundComponent->SetSound(LoopingSound);
		LoopingSoundComponent->Play();
	}

	SetLifeSpan(LifeSpanInSec);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.AddDynamic(this, &ThisClass::OnSphereHit);

	InitialPosition = GetActorLocation();
	DistanceSpanSqr = FMath::Square(DistanceSpan);

	InitProjectileMovementComponent();

	bIsEnding = false;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::End()
{
	ensureMsgf(bIsEnding, TEXT("Ends projectile without bIsEnding true"));
	if (!HasAuthority())
	{
		LoopingSoundComponent->Stop();
	}

	Sphere->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.RemoveDynamic(this, &ThisClass::OnSphereHit);

	ProjectileMovement->StopMovementImmediately();
}
