// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/UGeoGameplayLibrary.h"

FGeoAbilityTargetData UGeoProjectileAbility::BuildAbilityTargetData()
{
	FGeoAbilityTargetData Data = Super::BuildAbilityTargetData();
	Data.Origin = FVector2D(GetFireSocketLocation());
	return Data;
}

void UGeoProjectileAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	if (!GetCurrentActorInfo()->IsLocallyControlledPlayer())
	{
		// Server : don't spawn or end here — OnFireTargetDataReceived handles it when target data arrives.
		return;
	}

	SpawnProjectilesUsingTarget(AbilityTargetData.Yaw, FVector(AbilityTargetData.Origin, ArbitraryCharacterZ),
								AbilityTargetData.ServerSpawnTime);
	EndAbility(false, false);
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

	FGeoAbilityTargetData const* AbilityTargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	if (!ensureMsgf(AbilityTargetData, TEXT("GeoProjectileAbility: No FGeoAbilityTargetData in DataHandle — server projectile not spawned.")))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	// Server projectile spawn with updated values.
	SpawnProjectilesUsingTarget(AbilityTargetData->Yaw, FVector(AbilityTargetData->Origin, ArbitraryCharacterZ),
								AbilityTargetData->ServerSpawnTime);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
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
	TArray<FVector> const Directions = GeoASLib::GetTargetDirections(GetWorld(), Target, ProjectileYaw, Origin);
	for (FVector const& Direction : Directions)
	{
		SpawnProjectileUsingDirection(Direction, Origin, SpawnServerTime);
	}
}

void UGeoProjectileAbility::SpawnProjectile(FTransform const& SpawnTransform, float const SpawnServerTime) const
{
	checkf(ProjectileClass, TEXT("No ProjectileClass in the projectile spell!"));

	FPredictionKey PredictionKey;
	EGameplayAbilityActivationMode::Type const ActivationMode = GetCurrentActivationInfo().ActivationMode;
	if (ActivationMode == EGameplayAbilityActivationMode::Predicting
		|| ActivationMode == EGameplayAbilityActivationMode::Confirmed
		|| ActivationMode == EGameplayAbilityActivationMode::Authority)
	{
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	GeoASLib::FullySpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, StoredPayload, GetEffectDataArray(),
								   SpawnServerTime, PredictionKey);
}
