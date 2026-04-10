// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"

#include "DeployableSpawnerProjectile.generated.h"

/**
 * Projectile that spawns a deployable actor (turret, healing zone, etc.) when it lands.
 * Carries FDeployableDataParams and the deployable class set by the ability at fire time,
 * then hands them off to the spawned actor via InitInteractableData.
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
	virtual void EndProjectileLife() override;
	virtual void InitDeployable(AGeoDeployableBase* Deployable, AActor* PayloadOwner) const;
	void FillBaseData(FDeployableData& Data, AActor* PayloadOwner) const;

private:
	void SpawnDeployableActor();
};
