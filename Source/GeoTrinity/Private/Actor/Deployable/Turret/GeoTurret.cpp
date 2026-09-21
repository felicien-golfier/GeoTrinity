// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Turret/GeoTurret.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Net/UnrealNetwork.h"
#include "System/GeoBulletSubsystem.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoTurret::AGeoTurret(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileParams.OverrideDistanceSpan = EOverrideParam::OverrideValue;
	ProjectileParams.DistanceSpan = 2000.f;
	ProjectileParams.OverrideSpeed = EOverrideParam::OverrideValue;
	ProjectileParams.ProjectileSpeed = 4000.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::BeginPlay()
{
	Super::BeginPlay();

	if (IsPlayerTurret() || GeoLib::IsServer(GetWorld()))
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ThisClass::TryFire, FireInterval, true);
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
		this, GeoASLib::GetTeamId(this), TeamAttitudeMask::HostileOrNeutral, true);

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

	if (!ensureMsgf(ProjectileParams.ProjectileClass, TEXT("AGeoTurret: ProjectileParams.ProjectileClass is not set!")))
	{
		return;
	}

	float const Yaw = (CurrentTarget->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
	if (IsPlayerTurret())
	{
		Fire(Yaw);
	}
	else
	{
		MulticastFire(Yaw);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void AGeoTurret::MulticastFire_Implementation(float const Yaw)
{
	Fire(Yaw);
}

// ---------------------------------------------------------------------------------------------------------------------

void AGeoTurret::Fire(float const Yaw)
{
	FAbilityPayload Payload;
	Payload.SourceOwner = GetData()->Owner;
	Payload.SourceAvatar = GeoASLib::GetAvatarFromActor(GetData()->Owner);
	Payload.Origin = FVector2D(GetActorLocation());
	Payload.Yaw = Yaw;
	Payload.ServerSpawnTime = GeoLib::GetServerTime(GetWorld(), true);
	Payload.AbilityLevel = Data.Level;
	Payload.HitNotified = MakeShared<bool>(false);
	Payload.AbilityTag = GetData()->AbilityTag;

	UGeoBulletSubsystem::Get(GetWorld())
		->FireBullet(Payload, ProjectileParams, GetData()->EffectDataArray, TeamAttitudeMask::HostileOrNeutral,
					 /*bSeenThroughReplication*/ true);
}

// ---------------------------------------------------------------------------------------------------------------------

bool AGeoTurret::IsPlayerTurret() const
{
	return GeoASLib::GetTeamId(GetData()->Owner).GetId() == static_cast<uint8>(ETeam::Player);
}

// ---------------------------------------------------------------------------------------------------------------------

void AGeoTurret::Expire(bool const bForce)
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	FireTimerHandle.Invalidate();
	Super::Expire(bForce);
}
