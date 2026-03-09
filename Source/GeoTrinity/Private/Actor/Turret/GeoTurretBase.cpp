// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Turret/GeoTurretBase.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Tool/UGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoTurretBase::AGeoTurretBase()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCollisionProfileName(TEXT("GeoCapsule"));
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ScheduleFire();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::InitInteractableData(FInteractableActorData* Data)
{
	Super::InitInteractableData(Data);

	FTurretData* TurretData = static_cast<FTurretData*>(Data);
	EffectDataArray = TurretData->EffectDataArray;
	CharacterOwner = TurretData->CharacterOwner;
	TeamID = TurretData->TeamID;
	TurretLevel = TurretData->Level;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::OnRecalled()
{
	bWasBlinkingOnRecall = IsBlinking();
	Super::OnRecalled();
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoTurretBase::IsBlinking() const
{
	return GetDurationPercent() <= BlinkThreshold;
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* AGeoTurretBase::FindBestTarget() const
{
	TArray<AActor*> const HostileActors =
		UGeoAbilitySystemLibrary::GetAllAgentsWithRelationTowardsActor(this, this, ETeamAttitude::Hostile);
	return UGeoAbilitySystemLibrary::GetNearestActorFromList(this, HostileActors);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::ScheduleFire()
{
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ThisClass::TryFire, FireInterval, true);
}

// ---------------------------------------------------------------------------------------------------------------------

// What's left is Blueprint/data work:
// - Create BP subclasses of the new ability classes
// - Set up UEffectDataAsset slots (ammo cost GE, ammo restore GE, buff effects, recall normal/blink-bonus effects)
// - Create the GameplayCue BP for the recall beam VFX (RecallGameplayCueTag)
// - Create turret BP subclass with TurretProjectileClass and FireInterval set

void AGeoTurretBase::TryFire()
{
	if (!HasAuthority())
	{
		return;
	}

	AActor* Target = FindBestTarget();
	if (!IsValid(Target))
	{
		return;
	}

	ensureMsgf(TurretProjectileClass, TEXT("AGeoTurretBase: TurretProjectileClass is not set!"));
	if (!TurretProjectileClass)
	{
		return;
	}

	FVector const DirectionToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FTransform const SpawnTransform{DirectionToTarget.Rotation().Quaternion(), GetActorLocation()};

	float const SpawnServerTime = UGameplayLibrary::GetServerTime(GetWorld());

	FAbilityPayload Payload;
	Payload.Owner = this;
	Payload.Instigator = this;
	Payload.Origin = FVector2D(GetActorLocation());
	Payload.Yaw = DirectionToTarget.Rotation().Yaw;
	Payload.ServerSpawnTime = SpawnServerTime;
	Payload.AbilityLevel = FMath::RoundToInt(TurretLevel);

	UGameplayLibrary::SpawnProjectile(GetWorld(), TurretProjectileClass, SpawnTransform, Payload, EffectDataArray,
									  SpawnServerTime);
}
