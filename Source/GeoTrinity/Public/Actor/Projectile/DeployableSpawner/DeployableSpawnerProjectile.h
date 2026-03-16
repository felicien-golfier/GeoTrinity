// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"

#include "DeployableSpawnerProjectile.generated.h"

UCLASS()
class GEOTRINITY_API ADeployableSpawnerProjectile : public AGeoProjectile
{
	GENERATED_BODY()

public:
	float LifeDrain;

	virtual bool IsValidOverlap(AActor const* OtherActor) override;

protected:
	virtual void EndProjectileLife() override;
	virtual void InitDeployable(AGeoDeployableBase* Deployable, AActor* PayloadOwner) const;
	void FillBaseData(FDeployableData& Data, AActor* PayloadOwner) const;

	UPROPERTY(EditDefaultsOnly, Category = "Deployable")
	TSubclassOf<AGeoDeployableBase> DeployableActorClass;

private:
	void SpawnDeployableActor() const;
};
