// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoReloadAbility.generated.h"

class AGeoBuffPickup;
class UEffectDataAsset;

/**
 * Triangle reload ability: after a delay equal to FireDelay, restores ammo and spawns a buff pickup.
 * PowerScale = MissingAmmo / MaxAmmo — controls pickup size and effect magnitude.
 * A random buff is chosen from BuffEffectDataAssets each time.
 */
UCLASS()
class GEOTRINITY_API UGeoReloadAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	/** Binds the ammo-changed callback so the reload button is grayed out in the HUD when ammo is full. */
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Returns false when the character already has maximum ammo (reload is a no-op in that state). */
	virtual bool CheckCost(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
						   OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	/** Restores ammo via AmmoRestoreEffect and spawns a buff pickup at a random location near the character. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	TSubclassOf<UGameplayEffect> AmmoRestoreEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	TSubclassOf<AGeoBuffPickup> BuffPickupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	float MinSpawnRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Reload")
	float MaxSpawnRadius = 400.f;
};
