// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoReloadAbility.generated.h"

class AGeoBuffPickup;
class UEffectDataAsset;

/**
 * Triangle reload ability: after a delay equal to FireRate, restores ammo and spawns a buff pickup.
 * PowerScale = MissingAmmo / MaxAmmo — controls pickup size and effect magnitude.
 * A random buff is chosen from BuffEffectDataAssets each time.
 */
UCLASS()
class GEOTRINITY_API UGeoReloadAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reload")
	TSubclassOf<UGameplayEffect> AmmoRestoreEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reload")
	TSubclassOf<AGeoBuffPickup> BuffPickupClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reload")
	float MinSpawnRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reload")
	float MaxSpawnRadius = 400.f;
};
