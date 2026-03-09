// Copyright 2024 GeoTrinity. All Rights Reserved.

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

// ---
// 1. Create WBP_DeployChargeGauge
//
// - Parent class: GeoDeployChargeGaugeWidget
// - Add a Progress Bar in the designer
// - Right-click its Percent → Bind → Create Binding
// - In the function: DeployAbility → Get Charge Ratio → Return
// - That's the entire widget — nothing else needed
//
// ---
// 2. Player Character BP
//
// - Select DeployChargeGaugeComponent → Details → Widget Class = WBP_DeployChargeGauge
// - Adjust its position offset in the viewport if needed (attached to root by default)
// - CharacterWidgetComponent → leave Widget Class empty for now (player overhead health is "maybe later")
//
// ---
// 3. Enemy Character BP
//
// - Remove the manually added GeoCombattantWidgetComp you had in BP — it's now in C++, would be a duplicate
// - Select the C++-created CharacterWidgetComponent → Widget Class = your existing enemy health bar widget
// (WBP_EnemyHealthBar or whatever it's called)
//
// ---
// 4. BP_TurretBase (child of AGeoTurretBase)
//
// - Add a mesh component
// - Set TurretProjectileClass → an existing projectile BP
// - Set FireInterval, BlinkThreshold
// - Configure default health attributes
//
// ---
// 5. BP_TurretSpawnerProjectile (child of ATurretSpawnerProjectile)
//
// - Add a mesh
// - Set TurretActorClass = BP_TurretBase
//
// ---
// 6. BP_DeployTurretAbility (child of UGeoDeployAbility)
//
// - ProjectileClass = BP_TurretSpawnerProjectile
// - MinDeployDistance, MaxDeployDistance, MaxChargeTime
// - EffectDataAssets = damage effect data for turret bullets
// - No gauge code — fully automatic
//
// ---
// 7. Add UGeoDeployableManagerComponent to the player character BP
//
// - Add component, set MaxDeployables = 3 (or however many turrets Triangle can have)
//
// ---
// 8. DA_AbilityInfo
//
// - New entry: AbilityTag = Ability.Deploy, AbilityClass = BP_DeployTurretAbility, PlayerClass = Triangle

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
