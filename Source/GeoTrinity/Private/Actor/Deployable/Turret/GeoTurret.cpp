// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Turret/GeoTurret.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoTurret::AGeoTurret(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;

	ProjectileParams.OverrideDistanceSpan = EOverrideParam::OverrideValue;
	ProjectileParams.DistanceSpan = 2000.f;
	ProjectileParams.OverrideSpeed = EOverrideParam::OverrideValue;
	ProjectileParams.ProjectileSpeed = 4000.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ThisClass::TryFire, FireInterval, true);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoTurret, Data, COND_InitialOnly);
	DOREPLIFETIME(AGeoTurret, CurrentTarget);
}

void AGeoTurret::InitInteractable(FInteractableActorData* InputData)
{
	FDeployableData* DeployableData = static_cast<FDeployableData*>(InputData);
	ensureMsgf(DeployableData, TEXT("AGeoTurret: Data is not a FDeployableData!"));
	if (!DeployableData)
	{
		return;
	}

	Data = *DeployableData;

	Super::InitInteractable(InputData);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The server picks the target and replicates it; clients only orient toward the live target location.
	if (GeoLib::IsServer(GetWorld()))
	{
		CurrentTarget = FindBestTarget();
	}

	if (!IsValid(CurrentTarget))
	{
		return;
	}

	FVector const DirectionToTarget = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	SetActorRotation(DirectionToTarget.Rotation());
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* AGeoTurret::FindBestTarget() const
{
	TArray<AActor*> const HostileActors = UGeoAbilitySystemLibrary::GetInteractableActors(
		this, GeoASLib::GetTeamId(this), TeamAttitudeMask::HostileOrNeutral);

	if (UGeoAbilitySystemComponent* OwnerASC = GeoASLib::GetGeoAscFromActor(GetData()->Owner))
	{
		AActor* PreferredTarget = OwnerASC->GetLastBasicAbilityTarget();
		if (IsValid(PreferredTarget) && HostileActors.Contains(PreferredTarget))
		{
			return PreferredTarget;
		}
	}

	return UGeoAbilitySystemLibrary::GetNearestActorFromList(this, HostileActors);
}

// ---------------------------------------------------------------------------------------------------------------------

void AGeoTurret::TryFire()
{
	if (!IsValid(CurrentTarget))
	{
		return;
	}

	if (!ProjectileParams.ProjectileClass)
	{
		ensureMsgf(ProjectileParams.ProjectileClass, TEXT("AGeoTurret: ProjectileParams.ProjectileClass is not set!"));
		return;
	}

	FVector const DirectionToTarget = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FTransform const SpawnTransform{DirectionToTarget.Rotation().Quaternion(), GetActorLocation()};

	float const SpawnServerTime = GeoLib::GetServerTime(GetWorld());

	FAbilityPayload Payload;
	Payload.Owner = GetData()->Owner;
	Payload.Instigator = this;
	Payload.Origin = FVector2D(GetActorLocation());
	Payload.Yaw = DirectionToTarget.Rotation().Yaw;
	Payload.ServerSpawnTime = SpawnServerTime;
	Payload.AbilityLevel = Data.Level;
	Payload.AbilityTag = GetData()->AbilityTag;

	AGeoProjectile* Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), ProjectileParams, SpawnTransform, Payload,
																GetData()->EffectDataArray);
	if (!ensureMsgf(Projectile, TEXT("TurretProjectile: Failed to spawn projectile!")))
	{
		return;
	}

	GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, FPredictionKey());
}

// ---------------------------------------------------------------------------------------------------------------------

void AGeoTurret::Expire(float TimeBeforeDestroy)
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	FireTimerHandle.Invalidate();
	Super::Expire(TimeBeforeDestroy);
}
