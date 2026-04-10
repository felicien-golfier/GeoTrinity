// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoChargeBeamAbility.generated.h"

/**
 * Circle basic attack: hold to charge, release to fire a projectile.
 * Charge ratio (0–1) is encoded in the target data Seed and drives:
 *   - A lerped damage multiplier (MinDamageMultiplier to MaxDamageMultiplier)
 *   - A sweet spot bonus effect when released between SweetSpotMinRatio and SweetSpotMaxRatio
 */
UCLASS()
class GEOTRINITY_API UGeoChargeBeamAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

	UGeoChargeBeamAbility();

protected:
	virtual FGeoAbilityTargetData BuildAbilityTargetData() override;

	virtual TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const override;

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

	virtual float GetMaxChargeTime() const override { return MaxChargeTime; }

	// Sweet spot range (charge ratio 0–1). Releasing within this window applies SweetSpotBonusEffect.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float SweetSpotMinRatio = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float SweetSpotMaxRatio = 0.7f;

	// Damage multiplier lerped from Min (0% charge) to Max (100% charge).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float MinDamageMultiplier = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float MaxDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float MaxChargeTime = .5f;
};
