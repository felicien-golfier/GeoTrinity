// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
bool ADeployableSpawnerProjectile::IsValidOverlap(AActor* OtherActor)
{
	return OtherActor != GetInstigator() && Super::IsValidOverlap(OtherActor);
}

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
