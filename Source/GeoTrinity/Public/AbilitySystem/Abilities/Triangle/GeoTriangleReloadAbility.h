// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoTriangleReloadAbility.generated.h"

class AGeoBuffPickup;
class UEffectDataAsset;

/**
 * Triangle reload ability: after a delay equal to FireRate, restores ammo and spawns a buff pickup.
 * PowerScale = MissingAmmo / MaxAmmo — controls pickup size and effect magnitude.
 * A random buff is chosen from BuffEffectDataAssets each time.
 */
UCLASS()
class GEOTRINITY_API UGeoTriangleReloadAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reload")
	TArray<TSoftObjectPtr<UEffectDataAsset>> BuffEffectDataAssets;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reload")
	TSubclassOf<UGameplayEffect> AmmoRestoreEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reload")
	TSubclassOf<AGeoBuffPickup> BuffPickupClass;
};
