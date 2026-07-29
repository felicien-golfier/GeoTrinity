// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::EndProjectileLife()
{
	SpawnDeployableActor();
	Super::EndProjectileLife();
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::SpawnDeployableActor()
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	GeoASLib::FullySpawnDeployable(DeployableActorClass, Payload, EffectDataArray, Params, GetActorTransform());
}
