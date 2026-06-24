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
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::BeginPlay()
{
	Super::BeginPlay();

	if (GeoLib::IsServer(GetWorld()))
	{
		ScheduleFire();
	}
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
void AGeoTurret::ScheduleFire()
{
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ThisClass::TryFire, FireInterval, true);
}

// ---------------------------------------------------------------------------------------------------------------------

void AGeoTurret::TryFire()
{
	AActor* Target = FindBestTarget();
	if (!IsValid(Target))
	{
		return;
	}

	if (!TurretProjectileClass)
	{
		ensureMsgf(TurretProjectileClass, TEXT("AGeoTurret: TurretProjectileClass is not set!"));
		return;
	}

	FVector const DirectionToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
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

	GeoASLib::FullySpawnProjectile(GetWorld(), TurretProjectileClass, SpawnTransform, Payload,
								   GetData()->EffectDataArray, SpawnServerTime);
}

// ---------------------------------------------------------------------------------------------------------------------

void AGeoTurret::Expire(float TimeBeforeDestroy)
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	FireTimerHandle.Invalidate();
	Super::Expire(TimeBeforeDestroy);
}
