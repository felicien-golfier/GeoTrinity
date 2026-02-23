// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/UGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoDeployAbility::UGeoDeployAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo,
										FGameplayEventData const* TriggerEventData)
{
	// Call grandparent to commit and set up StoredPayload without scheduling fire
	UGeoGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	// Use local time (not server time) for accurate client-side charge duration measurement
	ChargeStartTime = GetWorld()->GetTimeSeconds();

	if (ActorInfo->IsLocallyControlledPlayer())
	{
		GetWorld()->GetTimerManager().SetTimer(ChargeAutoFireHandle, this, &UGeoDeployAbility::FireDeployable,
											   MaxChargeTime, false);
		OnChargeBegin(MaxChargeTime);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
								   FGameplayAbilityActivationInfo const ActivationInfo, bool const bReplicateEndAbility,
								   bool const bWasCancelled)
{
	if (ActorInfo->IsLocallyControlledPlayer())
	{
		GetWorld()->GetTimerManager().ClearTimer(ChargeAutoFireHandle);
		OnChargeEnded();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDeployAbility::GetChargeRatio() const
{
	if (MaxChargeTime <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartTime) / MaxChargeTime, 0.f, 1.f);
}

void UGeoDeployAbility::FireDeployable()
{
	float const ChargeTime = FMath::Min(GetWorld()->GetTimeSeconds() - ChargeStartTime, MaxChargeTime);
	float const ChargeRatio = MaxChargeTime > 0.f ? ChargeTime / MaxChargeTime : 0.f;
	PendingDeployDistance = FMath::Lerp(MinDeployDistance, MaxDeployDistance, ChargeRatio);
	Fire();
}
// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::InputReleased(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo)
{
	if (!bIsAbilityEnding && IsActive())
	{
		FireDeployable();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::Fire()
{
	// Encode deploy distance as integer cm in Seed so the server receives it
	StoredPayload.Seed = FMath::RoundToInt(PendingDeployDistance);
	SendFireDataToServer();

	if (!GetCurrentActorInfo()->IsLocallyControlledPlayer())
	{
		return;
	}

	// Let Server Spawn Deployable.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
												 FGameplayTag ApplicationTag)
{
	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	GetAbilitySystemComponentFromActorInfo()->ConsumeClientReplicatedTargetData(
		Handle, ActivationInfo.GetActivationPredictionKey());

	FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	ensureMsgf(TargetData, TEXT("No FGeoAbilityTargetData found in DataHandle for GeoDeployAbility"));

	float const DeployDistance = static_cast<float>(TargetData->Seed);
	FVector const Origin{TargetData->Origin, ActorInfo->AvatarActor->GetActorLocation().Z};

	SpawnDeployProjectile(Origin, TargetData->Yaw, TargetData->ServerSpawnTime, DeployDistance);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::SpawnDeployProjectile(FVector const& Origin, float const Yaw, float const SpawnServerTime,
											  float const DeployDistance) const
{
	checkf(ProjectileClass, TEXT("No ProjectileClass set on GeoDeployAbility!"));

	FTransform const SpawnTransform{FRotator(0.f, Yaw, 0.f).Quaternion(), Origin};

	FPredictionKey PredictionKey;
	EGameplayAbilityActivationMode::Type const ActivationMode = GetCurrentActivationInfo().ActivationMode;
	if (ActivationMode == EGameplayAbilityActivationMode::Predicting
		|| ActivationMode == EGameplayAbilityActivationMode::Confirmed
		|| ActivationMode == EGameplayAbilityActivationMode::Authority)
	{
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	AGeoProjectile* Projectile =
		UGameplayLibrary::SpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, StoredPayload,
										  GetEffectDataArray(), SpawnServerTime, PredictionKey);
	if (IsValid(Projectile))
	{
		Projectile->SetDistanceSpan(DeployDistance);
	}
}
