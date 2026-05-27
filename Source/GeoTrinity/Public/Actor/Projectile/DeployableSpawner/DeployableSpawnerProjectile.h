// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"

#include "DeployableSpawnerProjectile.generated.h"

/**
 * Projectile that spawns a deployable actor (turret, healing zone, etc.) when it lands.
 * Carries FDeployableDataParams and the deployable class set by the ability at fire time,
 * then hands them off to the spawned actor via InitInteractable.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API ADeployableSpawnerProjectile : public AGeoProjectile
{
	GENERATED_BODY()

public:
	FDeployableDataParams Params;
	TSubclassOf<AGeoDeployableBase> DeployableActorClass;

	/** Returns true only when OtherActor is the ground (static world geometry), triggering deployment. */
	virtual bool IsValidOverlap(AActor const* OtherActor) override;

protected:
	/** Spawns the deployable actor at the projectile's final position then destroys the projectile. */
	virtual void EndProjectileLife() override;
	/**
	 * Called after spawning to finish configuring the deployable with ability-specific data before FinishSpawning.
	 * Subclasses may override to set class-specific fields on the FDeployableData.
	 *
	 * @param Deployable    The freshly deferred-spawned deployable actor (before BeginPlay).
	 */
	virtual void InitDeployable(AGeoDeployableBase* Deployable) const;
	/** Spawns the deployable using the library helpers (StartSpawnDeployable → InitDeployable → FinishSpawnDeployable).
	 *  Returns true on success. */
	bool SpawnDeployableActor(AActor* PayloadOwner);
	/** Fills common FDeployableData fields (owner, level, team, seed, params, effects) from the projectile payload. */
	void FillBaseData(FDeployableData& Data) const;

private:
	void SpawnDeployableActor();
};
