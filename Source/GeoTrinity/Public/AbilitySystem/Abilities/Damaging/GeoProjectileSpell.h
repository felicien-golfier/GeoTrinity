// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeoDamageGameplayAbility.h"
#include "GeoProjectileSpell.generated.h"

class AGeoProjectile;
/**
 * A spell that launches a projectile
 */
UCLASS()
class GEOTRINITY_API UGeoProjectileSpell : public UGeoDamageGameplayAbility
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(FVector const& projectileTargetLocation);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;
};
