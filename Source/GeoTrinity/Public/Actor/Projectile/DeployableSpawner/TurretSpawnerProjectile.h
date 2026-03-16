// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"
#include "CoreMinimal.h"

#include "TurretSpawnerProjectile.generated.h"

UCLASS()
class GEOTRINITY_API ATurretSpawnerProjectile : public ADeployableSpawnerProjectile
{
	GENERATED_BODY()

protected:
	virtual void InitDeployable(AGeoDeployableBase* Deployable, AActor* PayloadOwner) const override;
};
