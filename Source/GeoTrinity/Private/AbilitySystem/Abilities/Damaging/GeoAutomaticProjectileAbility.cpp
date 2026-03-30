// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticProjectileAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/UGameplayLibrary.h"

FGeoAbilityTargetData UGeoAutomaticProjectileAbility::BuildAbilityTargetData()
{
	FGeoAbilityTargetData Data = Super::BuildAbilityTargetData();
	Data.Origin = FVector2D(GetFireSocketLocation());
	return Data;
}

bool UGeoAutomaticProjectileAbility::ExecuteShot_Implementation()
{
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[GeoAutomaticProjectileAbility] No ProjectileClass set!"));
		return false;
	}

	FPredictionKey PredictionKey;
	switch (GetCurrentActivationInfo().ActivationMode)
	{
	case EGameplayAbilityActivationMode::Predicting:
	case EGameplayAbilityActivationMode::Confirmed:
	case EGameplayAbilityActivationMode::Authority:
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
		break;
	case EGameplayAbilityActivationMode::Rejected:
		return false;
	case EGameplayAbilityActivationMode::NonAuthority:
		ensureMsgf(false, TEXT("Not sure that NonAuthority activation mode can even exist here"));
	default:
		PredictionKey = FPredictionKey();
		break;
	}

	FVector const Origin{StoredPayload.Origin, 0.f};
	AActor const* Avatar = GetAvatarActorFromActorInfo();
	float const ProjectileYaw = Avatar->GetActorRotation().Yaw;
	TArray<FVector> const Directions = UGameplayLibrary::GetTargetDirections(GetWorld(), Target, ProjectileYaw, Origin);

	bool bAnySpawned = false;
	for (FVector const& Direction : Directions)
	{
		FTransform const SpawnTransform{Direction.Rotation().Quaternion(), Origin};
		if (UGameplayLibrary::SpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, StoredPayload,
											  GetEffectDataArray(), StoredPayload.ServerSpawnTime, PredictionKey))
		{
			bAnySpawned = true;
		}
	}

	return bAnySpawned;
}
