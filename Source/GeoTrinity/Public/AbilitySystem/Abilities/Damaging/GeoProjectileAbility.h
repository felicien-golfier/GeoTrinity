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

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectileUsingDirection(FVector const& Direction, FVector const& Origin, float SpawnServerTime);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual void SpawnProjectile(FTransform const& SpawnTransform, float SpawnServerTime) const;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectilesUsingTarget(float ProjectileYaw, FVector const& Origin, float SpawnServerTime);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	EProjectileTarget Target;
};
