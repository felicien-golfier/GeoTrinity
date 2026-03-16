// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/DeployableSpawner/TurretSpawnerProjectile.h"

#include "Actor/Turret/GeoTurret.h"

// ---------------------------------------------------------------------------------------------------------------------
void ATurretSpawnerProjectile::InitDeployable(AGeoDeployableBase* Deployable, AActor* PayloadOwner) const
{
	FTurretData Data;
	FillBaseData(Data, PayloadOwner);
	Data.EffectDataArray = EffectDataArray;
	Deployable->InitInteractableData(&Data);
}
