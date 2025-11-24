// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeoProjectile.h"

#include "GeoTurretSpawnerProjectile.generated.h"

class AGeoTurretBase;
/**
 * An actor used to spawn a turret
 */
UCLASS()
class GEOTRINITY_API AGeoTurretSpawnerProjectile : public AGeoProjectile
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGeoTurretSpawnerProjectile();

	UFUNCTION(BlueprintNativeEvent, Category = "Turret")
	float GetTurretLevel() const;

	virtual bool IsValidOverlap(const AActor* OtherActor) override;

protected:
	virtual void EndProjectileLife() override;

	UPROPERTY(EditDefaultsOnly, Category = Turret)
	TSubclassOf<AGeoTurretBase> TurretActorClass;

private:
	void SpawnTurretActor() const;
};