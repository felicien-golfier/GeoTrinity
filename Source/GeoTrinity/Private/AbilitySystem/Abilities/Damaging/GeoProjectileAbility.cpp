// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/UGameplayLibrary.h"

void UGeoProjectileAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											FGameplayAbilityActorInfo const* ActorInfo,
											FGameplayAbilityActivationInfo const ActivationInfo,
											FGameplayEventData const* TriggerEventData)
{

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding) // We ended the ability in the Super.
	{
		return;
	}

	// Schedule fire with network delay compensation (plays montage on client, timer-only on server)
	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	ScheduleFireTrigger(ActivationInfo, AnimInstance);
}


void UGeoProjectileAbility::Fire()
{
	Super::Fire();

	if (GetCurrentActorInfo()->IsLocallyControlledPlayer())
	{
		AActor const* Avatar = GetAvatarActorFromActorInfo();
		SpawnProjectilesUsingTarget(Avatar->GetActorRotation().Yaw, Avatar->GetActorLocation(),
									UGameplayLibrary::GetServerTime(GetWorld(), true));
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}

void UGeoProjectileAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
													 FGameplayTag ApplicationTag)
{
	Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);

	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	GetAbilitySystemComponentFromActorInfo()->ConsumeClientReplicatedTargetData(
		Handle, ActivationInfo.GetActivationPredictionKey());

	FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	ensureMsgf(TargetData,
			   TEXT("No FGeoAbilityTargetData found in TriggerEventData, falling back to Generate a payload"));

	// Server projectile spawn with updated values.
	SpawnProjectilesUsingTarget(TargetData->Yaw,
								FVector(TargetData->Origin, ActorInfo->AvatarActor->GetActorLocation().Z),
								TargetData->ServerSpawnTime);
}

void UGeoProjectileAbility::SpawnProjectileUsingDirection(FVector const& Direction, FVector const& Origin,
														  float const SpawnServerTime)
{
	AActor const* Avatar = GetAvatarActorFromActorInfo();
	checkf(IsValid(Avatar), TEXT("Avatar Actor from actor info is invalid!"));

	FTransform SpawnTransform{Direction.Rotation().Quaternion(), Origin};

	SpawnProjectile(SpawnTransform, SpawnServerTime);
}

void UGeoProjectileAbility::SpawnProjectilesUsingTarget(float const ProjectileYaw, FVector const& Origin,
														float const SpawnServerTime)
{
	TArray<FVector> const Directions = UGameplayLibrary::GetTargetDirections(GetWorld(), Target, ProjectileYaw, Origin);
	for (FVector const& Direction : Directions)
	{
		SpawnProjectileUsingDirection(Direction, Origin, SpawnServerTime);
	}
}

void UGeoProjectileAbility::SpawnProjectile(FTransform const& SpawnTransform, float const SpawnServerTime) const
{
	checkf(ProjectileClass, TEXT("No ProjectileClass in the projectile spell!"));

	// Pass the prediction key only during the initial predicted activation.
	// In Confirmed mode (continuous fire shots 2+), the key is stale — skip fake spawning.
	FPredictionKey PredictionKey;
	switch (GetCurrentActivationInfo().ActivationMode)
	{
	case EGameplayAbilityActivationMode::Predicting:
	case EGameplayAbilityActivationMode::Confirmed:
	case EGameplayAbilityActivationMode::Authority:
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
		break;
	case EGameplayAbilityActivationMode::Rejected:
		return;
	case EGameplayAbilityActivationMode::NonAuthority:
		ensureMsgf(false, TEXT("Not sure that NonAuthority activation mode can even exist here"));
	default:
		PredictionKey = FPredictionKey();
		break;
	}

	UGameplayLibrary::SpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, StoredPayload, GetEffectDataArray(),
									  SpawnServerTime, PredictionKey);
}
