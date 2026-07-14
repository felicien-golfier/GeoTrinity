// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"

UGeoDeployAbility::UGeoDeployAbility()
{
	FireMode = EFireMode::ChargeForFireDelay;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDeployAbility::CanActivateAbility(FGameplayAbilitySpecHandle const Handle,
										   FGameplayAbilityActorInfo const* ActorInfo,
										   FGameplayTagContainer const* SourceTags,
										   FGameplayTagContainer const* TargetTags,
										   FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (CurrentCharges <= 0)
	{
		return false;
	}

	if (GetWorld()->GetTimeSeconds() - LastActivationWorldTime < MinTimeBetweenDeploys)
	{
		return false;
	}

	UGeoDeployableManagerComponent* DeployableManager =
		ActorInfo->AvatarActor->GetComponentByClass<UGeoDeployableManagerComponent>();
	ensureMsgf(DeployableManager, TEXT("GeoDeployAbility: No UGeoDeployableManagerComponent on avatar '%s'!"),
			   *GetNameSafe(ActorInfo->AvatarActor.Get()));
	if (!DeployableManager)
	{
		return false;
	}

	return DeployableManager->CanDeploy(DeployableActorClass);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo,
										FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Super may have already ended the ability (e.g. a failed cost commit) — don't spend a charge on a no-op.
	if (!bIsAbilityEnding && IsActive())
	{
		ConsumeCharge();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	CurrentCharges = MaxCharges;
	NextChargeReadyWorldTime = 0.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle,
															FGameplayAbilityActorInfo const* ActorInfo,
															float& TimeRemaining, float& CooldownDuration) const
{
	CooldownDuration = MinTimeBetweenDeploys;
	TimeRemaining = FMath::Max(MinTimeBetweenDeploys - (GetWorld()->GetTimeSeconds() - LastActivationWorldTime), 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::GetChargeRechargeTimeRemainingAndDuration(float& OutTimeRemaining, float& OutDuration) const
{
	OutDuration = ChargeRechargeTime;
	OutTimeRemaining =
		CurrentCharges >= MaxCharges ? 0.f : FMath::Max(NextChargeReadyWorldTime - GetWorld()->GetTimeSeconds(), 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::ConsumeCharge()
{
	CurrentCharges = FMath::Max(CurrentCharges - 1, 0);
	LastActivationWorldTime = GetWorld()->GetTimeSeconds();
	StartRechargeTimerIfNeeded();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::StartRechargeTimerIfNeeded()
{
	if (CurrentCharges >= MaxCharges || RechargeTimerHandle.IsValid())
	{
		return;
	}

	NextChargeReadyWorldTime = GetWorld()->GetTimeSeconds() + ChargeRechargeTime;
	GetWorld()->GetTimerManager().SetTimer(RechargeTimerHandle, this, &UGeoDeployAbility::OnChargeRecharged,
											ChargeRechargeTime, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::OnChargeRecharged()
{
	RechargeTimerHandle.Invalidate();
	CurrentCharges = FMath::Min(CurrentCharges + 1, MaxCharges);
	StartRechargeTimerIfNeeded();
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoAbilityTargetData UGeoDeployAbility::GetUpdatedTargetData()
{
	float PendingDeployDistance = FMath::Lerp(MinDeployDistance, MaxDeployDistance, GetChargeRatio());
	// Encode deploy distance as integer cm in Seed so the server receives it
	StoredPayload.Seed = FMath::RoundToInt(PendingDeployDistance);
	return Super::GetUpdatedTargetData();
}


// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::SpawnProjectile(FTransform const& SpawnTransform, float const SpawnServerTime) const
{
	checkf(ProjectileClass, TEXT("No ProjectileClass set on GeoDeployAbility!"));

	FPredictionKey PredictionKey;
	EGameplayAbilityActivationMode::Type const ActivationMode = GetCurrentActivationInfo().ActivationMode;
	if (ActivationMode == EGameplayAbilityActivationMode::Predicting
		|| ActivationMode == EGameplayAbilityActivationMode::Confirmed
		|| ActivationMode == EGameplayAbilityActivationMode::Authority)
	{
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	AGeoProjectile* Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform,
																StoredPayload, GetEffectDataArray(), PredictionKey);
	if (!IsValid(Projectile))
	{
		ensureMsgf(false, TEXT("GeoDeployAbility: Failed to spawn projectile!"));
		return;
	}

	Projectile->OverrideDistanceSpan(StoredPayload.Seed);
	ADeployableSpawnerProjectile* DeployableSpawnerProjectile = Cast<ADeployableSpawnerProjectile>(Projectile);
	checkf(DeployableSpawnerProjectile, TEXT("SpawnerProjectile  must be a ADeployableSpawnerProjectile"));
	DeployableSpawnerProjectile->Params = Params;
	DeployableSpawnerProjectile->DeployableActorClass = DeployableActorClass;

	GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, PredictionKey);
}
