// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticProjectileAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/UGeoGameplayLibrary.h"

bool UGeoAutomaticProjectileAbility::ExecuteShot_Implementation()
{
	if (!ProjectileParams.ProjectileClass)
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

	FVector const Origin{StoredPayload.Origin, ArbitraryCharacterZ};
	AActor const* Avatar = GetAvatarActorFromActorInfo();
	float const ProjectileYaw = StoredPayload.Yaw;
	TArray<FVector> const Directions = GeoASLib::GetTargetDirections(GetWorld(), Target, ProjectileYaw, Origin);

	bool bAnySpawned = false;
	for (FVector const& Direction : Directions)
	{
		FTransform const SpawnTransform{Direction.Rotation().Quaternion(), Origin};
		AGeoProjectile* Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), ProjectileParams, SpawnTransform,
																	StoredPayload, GetEffectDataArray(), PredictionKey);
		if (!ensureMsgf(Projectile, TEXT("GeoAutomaticProjectileAbility: Failed to spawn projectile!")))
		{
			continue;
		}

		GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, StoredPayload.ServerSpawnTime,
										PredictionKey);
		bAnySpawned = true;
	}

	return bAnySpawned;
}
