// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Projectile/GeoProjectile.h"

#include "AbilitySystem/Data/EffectData.h"   //Necessary for array transfer.
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

using GeoASL = UGeoAbilitySystemLibrary;

// ---------------------------------------------------------------------------------------------------------------------
AGeoProjectile::AGeoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = false;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);

	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);

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
bool AGeoProjectile::IsValidOverlap(const AActor* OtherActor)
{
	if (bIsEnding)
	{
		return false;
	}

	UGeoAbilitySystemComponent* SourceASC = Payload.Instigator->GetComponentByClass<UGeoAbilitySystemComponent>();
	if (!IsValid(SourceASC))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("A projectile was launched with an invalid Source ASC, this should never happen"));
		return false;
	}

	const AActor* SourceAvatarActor = SourceASC->GetAvatarActor();

	// Don't apply on self
	if (!IsValid(SourceAvatarActor) || (SourceAvatarActor == OtherActor))
	{
		return false;
	}

	const IGenericTeamAgentInterface* TeamInterface = nullptr;
	if (!GameplayLibrary::GetTeamInterface(OtherActor, TeamInterface))
	{
		return false;
	}

	// TODO: use TeamInterface->GetTeamAttitudeTowards()
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
		GeoASL::ApplyEffectFromEffectData(EffectDataArray, GeoASL::GetGeoAscFromActor(Payload.Instigator),
			GeoASL::GetGeoAscFromActor(OtherActor), Payload.AbilityLevel, Payload.Seed);
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

	OnProjectileEndLifeDelegate.Broadcast(this);
}

void AGeoProjectile::InitProjectileMovementComponent()
{
	// Clear any previous movement state
	ProjectileMovement->SetUpdatedComponent(GetRootComponent());

	if (ProjectileMovement->InitialSpeed > 0.f)
	{
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
		if (ProjectileMovement->bRotationFollowsVelocity)
		{
			SetActorRotation(ProjectileMovement->Velocity.Rotation());
		}
		ProjectileMovement->UpdateComponentVelocity();
	}
	else
	{
		ProjectileMovement->StopMovementImmediately();
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
	Sphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnSphereHit);

	InitialPosition = GetActorLocation();
	DistanceSpanSqr = FMath::Square(DistanceSpan);

	InitProjectileMovementComponent();

	bIsEnding = false;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::End()
{
	ensureMsgf(bIsEnding || !HasAuthority(), TEXT("Ends projectile on server without bIsEnding true"));

	if (!HasAuthority())
	{
		LoopingSoundComponent->Stop();
	}

	Sphere->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.RemoveDynamic(this, &ThisClass::OnSphereHit);

	ProjectileMovement->StopMovementImmediately();
}
