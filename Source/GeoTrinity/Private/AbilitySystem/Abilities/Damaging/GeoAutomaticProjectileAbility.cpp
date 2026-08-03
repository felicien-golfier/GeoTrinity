// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticProjectileAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoAutomaticProjectileAbility::RemoteFireShot(AActor* Avatar, UGeoAbilitySystemComponent* SourceASC) const
{
	SpawnRemoteProjectiles(Avatar, SourceASC, ProjectileParams, Target);
}

bool UGeoAutomaticProjectileAbility::ExecuteShot_Implementation()
{
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
	return GeoASLib::SpawnProjectileSpread(GetWorld(), ProjectileParams, Target, Origin, StoredPayload.Yaw,
										   StoredPayload.ServerSpawnTime, StoredPayload, GetEffectDataArray(),
										   PredictionKey)
		> 0;
}
