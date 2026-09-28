// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::EndProjectileLife()
{
	SpawnDeployableActor();
	Super::EndProjectileLife();
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::SpawnDeployableActor()
{
	if (bVisualOnly)
	{
		return;
	}

	GeoASLib::FullySpawnDeployable(DeployableActorClass, Payload, EffectDataArray, Params, GetActorTransform());
}
