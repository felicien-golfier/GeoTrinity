// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoPlaceDeployableAbility.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

UGeoPlaceDeployableAbility::UGeoPlaceDeployableAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGeoPlaceDeployableAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	if (ensureMsgf(DeployableActorClass, TEXT("%s: DeployableActorClass is not set"), *GetName()))
	{
		FDeployableDataParams SpawnParams = Params;
		float const LevelSize = Size.GetValueAtLevel(StoredPayload.AbilityLevel);
		if (LevelSize > 0.f)
		{
			SpawnParams.Size = LevelSize;
		}

		for (FVector2D const& Location : GeoASLib::GetTargetLocations(Target, MakeTargetContext()))
		{
			GeoASLib::FullySpawnDeployable(DeployableActorClass, StoredPayload, GetEffectDataArray(), SpawnParams,
										   FTransform(FVector(Location, 0.f)));
		}
	}

	EndAbility();
}
