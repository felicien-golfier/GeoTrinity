// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/GameplayLibrary.h"

void UGeoProjectileAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											FGameplayAbilityActorInfo const* ActorInfo,
											FGameplayAbilityActivationInfo const ActivationInfo,
											FGameplayEventData const* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Build payload from avatar transform
	AActor* Instigator = GetAvatarActorFromActorInfo();
	if (FGeoAbilityTargetData const* TargetData =
			static_cast<FGeoAbilityTargetData const*>(TriggerEventData->TargetData.Get(0)))
	{
		StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, TargetData->Origin,
											 TargetData->Yaw, TargetData->ServerSpawnTime, TargetData->Seed);
	}
	else
	{
		StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, Instigator->GetTransform());
	}

	// Extract orientation from TriggerEventData if available (sent via event-based activation)
	// Both client and server receive the same event data in a single RPC

	ensureMsgf(TriggerEventData && TriggerEventData->TargetData.Num() > 0, TEXT("No TargetData in TriggerEventData!"));


	// Schedule fire with network delay compensation (plays montage on client, timer-only on server)
	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	ScheduleFireTrigger(ActivationInfo, AnimInstance, StoredPayload.ServerSpawnTime);
}


void UGeoProjectileAbility::Fire()
{
	Super::Fire();

	SpawnProjectilesUsingTarget();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}

void UGeoProjectileAbility::SpawnProjectileUsingDirection(FVector const& Direction)
{
	AActor const* Avatar = GetAvatarActorFromActorInfo();
	checkf(IsValid(Avatar), TEXT("Avatar Actor from actor info is invalid!"));

	FTransform SpawnTransform{Direction.Rotation().Quaternion(), Avatar->GetActorLocation()};

	SpawnProjectile(SpawnTransform);
}

void UGeoProjectileAbility::SpawnProjectilesUsingTarget()
{
	AActor const* Avatar = GetAvatarActorFromActorInfo();
	float const ProjectileYaw = Avatar->GetActorRotation().Yaw;

	TArray<FVector> const Directions =
		GameplayLibrary::GetTargetDirections(GetWorld(), Target, ProjectileYaw, Avatar->GetActorLocation());
	for (FVector const& Direction : Directions)
	{
		SpawnProjectileUsingDirection(Direction);
	}
}

void UGeoProjectileAbility::SpawnProjectile(FTransform const SpawnTransform) const
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

	GameplayLibrary::SpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, StoredPayload, GetEffectDataArray(),
									 FireRate, PredictionKey);
}
