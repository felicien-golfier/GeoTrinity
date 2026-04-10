// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

#include "GeoProjectileAbility.generated.h"

enum class EProjectileTarget : uint8;
class AGeoProjectile;
/**
 * A spell that launches a projectile
 */
UCLASS()
class GEOTRINITY_API UGeoProjectileAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual FGeoAbilityTargetData BuildAbilityTargetData() override;

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

	/**
	 * Spawns a projectile aimed along Direction from Origin.
	 *
	 * @param Direction        Normalized world-space fire direction.
	 * @param Origin           World-space spawn location.
	 * @param SpawnServerTime  Synchronized server time used to fast-forward the projectile on the server.
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectileUsingDirection(FVector const& Direction, FVector const& Origin, float SpawnServerTime);

	/**
	 * Spawns a projectile at SpawnTransform. Override to change the projectile class or initialization.
	 *
	 * @param SpawnTransform   World transform for the new projectile.
	 * @param SpawnServerTime  Synchronized server time for position fast-forwarding.
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual void SpawnProjectile(FTransform const& SpawnTransform, float SpawnServerTime) const;

	/**
	 * Spawns one projectile per direction returned by GetTargetDirections for the given Target mode.
	 *
	 * @param ProjectileYaw    Yaw angle used for Forward mode targeting.
	 * @param Origin           World-space origin for direction calculations.
	 * @param SpawnServerTime  Synchronized server time for position fast-forwarding.
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectilesUsingTarget(float ProjectileYaw, FVector const& Origin, float SpawnServerTime);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	EProjectileTarget Target;
};
