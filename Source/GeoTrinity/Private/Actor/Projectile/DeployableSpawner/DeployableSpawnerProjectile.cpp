// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"

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

	AGeoDeployableBase* Deployable =
		GeoASLib::FullySpawnDeployable(DeployableActorClass, Payload, EffectDataArray, Params, GetActorTransform());
	UGeoAbilitySystemComponent* OwnerASC = GeoASLib::GetGeoAscFromActor(Payload.SourceOwner);
	if (ensureMsgf(IsValid(Deployable), TEXT("%hs: invalid Deployable"), __FUNCTION__) && OwnerASC)
	{
		OwnerASC->OnDeployableLanded.Broadcast(*Deployable);
	}
}
