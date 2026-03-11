// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeoProjectile.h"

#include "TurretSpawnerProjectile.generated.h"

class AGeoTurret;
/**
 * An actor used to spawn a turret
 */
UCLASS()
class GEOTRINITY_API ATurretSpawnerProjectile : public AGeoProjectile
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATurretSpawnerProjectile();

	UFUNCTION(BlueprintNativeEvent, Category = "Turret")
	float GetTurretLevel() const;

	virtual bool IsValidOverlap(AActor const* OtherActor) override;

protected:
	virtual void EndProjectileLife() override;

	UPROPERTY(EditDefaultsOnly, Category = Turret)
	TSubclassOf<AGeoTurret> TurretActorClass;

private:
	void SpawnTurretActor() const;
};
