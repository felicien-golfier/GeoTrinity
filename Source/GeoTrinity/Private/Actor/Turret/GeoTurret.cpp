// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Turret/GeoTurret.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::BeginPlay()
{
	Super::BeginPlay();

	ScheduleFire();
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
}

void AGeoTurret::InitInteractableData(FInteractableActorData* InputData)
{
	FTurretData* TurretData = static_cast<FTurretData*>(InputData);
	ensureMsgf(TurretData, TEXT("AGeoTurret: Data is not a FTurretData!"));
	if (!TurretData)
	{
		return;
	}

	Data = *TurretData;

	Super::InitInteractableData(InputData);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurret::OnRecalled()
{
	bWasBlinkingOnRecall = IsBlinking();
	Super::OnRecalled();
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoTurret::IsBlinking() const
{
	return GetDurationPercent() <= BlinkThreshold;
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* AGeoTurret::FindBestTarget() const
{
	TArray<AActor*> const HostileActors =
		UGeoAbilitySystemLibrary::GetAllAgentsWithRelationTowardsActor(this, this, ETeamAttitude::Hostile);
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

	float const SpawnServerTime = UGameplayLibrary::GetServerTime(GetWorld());

	FAbilityPayload Payload;
	Payload.Owner = GetData()->CharacterOwner;
	Payload.Instigator = this;
	Payload.Origin = FVector2D(GetActorLocation());
	Payload.Yaw = DirectionToTarget.Rotation().Yaw;
	Payload.ServerSpawnTime = SpawnServerTime;
	Payload.AbilityLevel = FMath::RoundToInt(Data.Level);

	UGameplayLibrary::SpawnProjectile(GetWorld(), TurretProjectileClass, SpawnTransform, Payload,
									  GetData()->EffectDataArray, SpawnServerTime);
	SetActorRotation(DirectionToTarget.Rotation());
}
