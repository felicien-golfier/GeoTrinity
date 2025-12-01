// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeoDamageGameplayAbility.h"

#include "GeoProjectileAbility.generated.h"

UENUM(BlueprintType)
enum class ETarget : uint8
{
	Forward,
	AllPlayers
};

class AGeoProjectile;
/**
 * A spell that launches a projectile
 */
UCLASS()
class GEOTRINITY_API UGeoProjectileAbility : public UGeoDamageGameplayAbility
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectileUsingLocation(const FVector& projectileTargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(const FRotator& DirectionRotator);
	
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectilesUsingTarget();

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	TArray<FVector> GetTargetLocations();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ETarget Target = ETarget::Forward;
};
