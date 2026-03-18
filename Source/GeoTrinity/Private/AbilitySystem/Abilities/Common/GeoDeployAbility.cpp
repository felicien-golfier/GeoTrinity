// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoDeployAbility::UGeoDeployAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
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

	UGeoDeployableManagerComponent* DeployableManager =
		ActorInfo->AvatarActor->GetComponentByClass<UGeoDeployableManagerComponent>();
	ensureMsgf(DeployableManager, TEXT("GeoDeployAbility: No UGeoDeployableManagerComponent on avatar '%s'!"),
			   *GetNameSafe(ActorInfo->AvatarActor.Get()));
	if (!DeployableManager)
	{
		return false;
	}

	return DeployableManager->CanDeploy();
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
		float const MaxChargeTime = GetDefault<UGameDataSettings>()->DeployMaxChargeTime;
		GetWorld()->GetTimerManager().SetTimer(ChargeAutoFireHandle, this, &UGeoDeployAbility::FireDeployable,
											   MaxChargeTime, false);

		if (APlayableCharacter* Character = Cast<APlayableCharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->ShowDeployChargeGauge(this);
		}
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

		if (APlayableCharacter* Character = Cast<APlayableCharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->HideDeployChargeGauge();
		}
		OnChargeEnded();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDeployAbility::GetRawChargeRatio() const
{
	float const MaxChargeTime = GetDefault<UGameDataSettings>()->DeployMaxChargeTime;
	if (MaxChargeTime <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartTime) / MaxChargeTime, 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDeployAbility::ApplyChargeCurve(float const RawRatio) const
{
	APlayableCharacter const* Character = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor.Get());
	ensureMsgf(
		Character,
		TEXT(
			"GeoDeployAbility: AvatarActor '%s' is not an APlayableCharacter — this ability must only be granted to playable characters."),
		*GetNameSafe(GetCurrentActorInfo()->AvatarActor.Get()));
	if (!Character)
	{
		return RawRatio;
	}

	UCurveFloat const* Curve = Character->GetGaugeChargingSpeedCurve();
	ensureMsgf(
		Curve,
		TEXT("GeoDeployAbility: GaugeChargingSpeedCurve is not set on '%s'. Assign a CurveFloat in the character BP."),
		*Character->GetName());
	if (!Curve)
	{
		return RawRatio;
	}

	return FMath::Clamp(Curve->GetFloatValue(RawRatio), 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDeployAbility::GetChargeRatio() const
{
	return ApplyChargeCurve(GetRawChargeRatio());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::FireDeployable()
{
	PendingDeployDistance = FMath::Lerp(MinDeployDistance, MaxDeployDistance, ApplyChargeCurve(GetRawChargeRatio()));
	BuildDataAndFire();
}
// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	SendFireDataToServer(AbilityTargetData);

	if (!GetCurrentActorInfo()->IsLocallyControlledPlayer())
	{
		return;
	}

	FVector const Origin{AbilityTargetData.Origin, ArbitraryCharacterZ};
	SpawnDeployProjectile(Origin, AbilityTargetData.Yaw, AbilityTargetData.ServerSpawnTime, PendingDeployDistance);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
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
FGeoAbilityTargetData UGeoDeployAbility::BuildAbilityTargetData()
{
	// Encode deploy distance as integer cm in Seed so the server receives it
	StoredPayload.Seed = FMath::RoundToInt(PendingDeployDistance);
	FGeoAbilityTargetData AbilityTargetData = Super::BuildAbilityTargetData();
	AbilityTargetData.Seed = StoredPayload.Seed; // Ensure to set it properly even if Super changes code.
	return AbilityTargetData;
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

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
		if (ADeployableSpawnerProjectile* DeployableSpawnerProjectile = Cast<ADeployableSpawnerProjectile>(Projectile))
		{
			DeployableSpawnerProjectile->LifeDrain = LifeDrain;
		}
	}
}
