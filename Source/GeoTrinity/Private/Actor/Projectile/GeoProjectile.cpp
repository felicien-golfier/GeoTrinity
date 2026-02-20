// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Projectile/GeoProjectile.h"

#include "AbilitySystem/Data/EffectData.h" //Necessary for array transfer.
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/UGameplayLibrary.h"

using GeoASL = UGeoAbilitySystemLibrary;

static TAutoConsoleVariable CVarDrawServerProjectiles(TEXT("Geo.DrawServerProjectiles"), false,
													  TEXT("Draw debug spheres for projectiles on the server"));

static TAutoConsoleVariable CVarLocalOnlyProjectiles(
	TEXT("Geo.LocalOnlyProjectiles"), false,
	TEXT(
		"When true, owning client sees only its local predicted projectile (server projectile not replicated to owner)"));

// ---------------------------------------------------------------------------------------------------------------------
AGeoProjectile::AGeoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);

	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_GeoCharacter, ECR_Overlap);

	Sphere->SetCollisionObjectType(ECC_GeoCharacter);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	LoopingSoundComponent = CreateDefaultSubobject<UAudioComponent>("LoopingSoundComponent");
	LoopingSoundComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoProjectile, PredictionKeyId);
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoProjectile::IsNetRelevantFor(AActor const* RealViewer, AActor const* ViewTarget,
									  FVector const& SrcLocation) const
{
	// Decides if yes or not it needs to replicate on this client.
	// Here we don't want to replicate on the projectile owner.
	if (CVarLocalOnlyProjectiles.GetValueOnGameThread() && PredictionKeyId != 0 && GetNetConnection()
		&& GetNetConnection() == RealViewer->GetNetConnection())
	{
		return false;
	}

	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::BeginPlay()
{
	Super::BeginPlay();

	// When a real (server-replicated) projectile arrives on the owning client,
	// find and destroy the matching predicted fake spawned earlier by the client.
	if (CVarLocalOnlyProjectiles.GetValueOnGameThread() || PredictionKeyId == 0
		|| UGameplayLibrary::IsServer(GetWorld()) || Implements<UGeoPoolableInterface>())
	{
		return;
	}

	for (TActorIterator<AGeoProjectile> It(GetWorld(), GetClass()); It; ++It)
	{
		AGeoProjectile* Other = *It;
		if (Other != this && Other->PredictionKeyId == PredictionKeyId && Other->GetIsReplicated()
			&& Other->GetOwner() == GetOwner())
		{
			Other->Destroy();
		}
	}
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

	float const ElapsedDistanceSqr = FVector::DistSquared(GetActorLocation(), InitialPosition);
	if (ElapsedDistanceSqr >= DistanceSpanSqr)
	{
		bIsEnding = true;
		EndProjectileLife();
	}

	UE_VLOG_SPHERE(this, LogGeoTrinity, Verbose, GetActorLocation(), GetSimpleCollisionRadius(),
				   UGameplayLibrary::GetColorForObject(GetOuter()), TEXT("Projectile tick of %s"), *GetName());

	if (CVarDrawServerProjectiles.GetValueOnGameThread() && UGameplayLibrary::IsServer(GetWorld()))
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), GetSimpleCollisionRadius(), 8,
						UGameplayLibrary::GetColorForObject(GetOuter()), false, 0.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoProjectile::IsValidOverlap(AActor const* OtherActor)
{
	if (bIsEnding)
	{
		return false;
	}

	UGeoAbilitySystemComponent* SourceASC = Payload.Owner->GetComponentByClass<UGeoAbilitySystemComponent>();
	if (!IsValid(SourceASC))
	{
		UE_LOG(LogGeoTrinity, Error,
			   TEXT("A projectile was launched with an invalid Source ASC, this should never happen"));
		return false;
	}

	AActor const* SourceAvatarActor = SourceASC->GetAvatarActor();

	// Don't apply on self
	if (!IsValid(SourceAvatarActor) || (SourceAvatarActor == OtherActor))
	{
		return false;
	}

	IGenericTeamAgentInterface const* TeamInterface = nullptr;
	if (!UGameplayLibrary::GetTeamInterface(OtherActor, TeamInterface))
	{
		return false;
	}

	// TODO: use TeamInterface->GetTeamAttitudeTowards()
	return TeamInterface->GetGenericTeamId().GetId() != FGenericTeamId::NoTeam
		&& (TeamInterface->GetGenericTeamId().GetId() & ApplyEffectToTeamOnOverlap) != 0x00;
}
// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									 UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex,
									 bool bFromSweep, FHitResult const& SweepResult)
{
	if (!IsValidOverlap(OtherActor))
	{
		return;
	}

	bIsEnding = true;

	if (HasAuthority())
	{
		GeoASL::ApplyEffectFromEffectData(EffectDataArray, GeoASL::GetGeoAscFromActor(Payload.Instigator),
										  GeoASL::GetGeoAscFromActor(OtherActor), Payload.AbilityLevel, Payload.Seed);
	}

	EndProjectileLife();
}

void AGeoProjectile::OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
								 FVector NormalImpulse, FHitResult const& Hit)

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

	FVector const actorLocation = GetActorLocation();
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

	OnProjectileEndLifeDelegate.Broadcast(this);
	Destroy();
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
void AGeoProjectile::AdvanceProjectile(float const TimeDelta)
{
	if (TimeDelta <= 0.f || bIsEnding)
	{
		return;
	}

	FVector const CurrentLocation = GetActorLocation();
	FVector const Velocity = ProjectileMovement->Velocity;
	FVector const AdvancedPosition = CurrentLocation + Velocity * TimeDelta;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Payload.Instigator);

	FCollisionObjectQueryParams ObjectQueryParams = FCollisionObjectQueryParams(ECC_GeoCharacter); // GeoCharacter ECC
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	TArray<FHitResult> HitResults;
	bool const bHit = GetWorld()->LineTraceMultiByObjectType(HitResults, CurrentLocation, AdvancedPosition,
															 ObjectQueryParams, QueryParams);

	if (bHit)
	{
		for (FHitResult const& Hit : HitResults)
		{
			if (AActor* HitActor = Hit.GetActor(); HitActor && IsValidOverlap(HitActor))
			{
				SetActorLocation(Hit.ImpactPoint);
				OnSphereOverlap(nullptr, HitActor, nullptr, 0, false, Hit);
				return;
			}
		}
	}

	SetActorLocation(AdvancedPosition);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::InitProjectileLife()
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
