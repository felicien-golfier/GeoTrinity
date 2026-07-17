// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoSweetSpotChargePassiveAbility.generated.h"

class UTexture2D;

/**
 * Passive ability for the Circle player.
 * Listens to OnHealProvided on the owner's ASC (server): every heal provided accumulates into the HealCharge attribute
 * (capped at HealRequiredForFullCharge) while the HUD status-bar gauge fills bottom-to-top. Once the cap is reached
 * the gauge is full (shining GaugeFullColor) and the next charge-beam sweet-spot release adds
 * GetHealsToDamageMultiplier to its damage multiplier: a boost lerped from EdgeDamageMultiplierBoost to
 * CenterDamageMultiplierBoost by how close the release lands to the sweet-spot center. The release then consumes the
 * gauge (UGeoChargeBeamAbility::DealDamage → ConsumeGauge).
 */
UCLASS()
class GEOTRINITY_API UGeoSweetSpotChargePassiveAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	UGeoSweetSpotChargePassiveAbility();

	/** Returns the gauge fill (0–1): recorded HealCharge over HealRequiredForFullCharge, clamped. */
	float GetGaugeRatio(UAbilitySystemComponent const& ASC) const;

	/** Returns the damage-multiplier boost a full-gauge sweet-spot release adds: lerped from EdgeDamageMultiplierBoost
	 * to CenterDamageMultiplierBoost by SweetSpotPrecision (0 = window edge, 1 = dead center). */
	float GetHealsToDamageMultiplier(float SweetSpotPrecision) const;

	/** Zeroes HealCharge, so the gauge starts charging from empty again. Server only. */
	void ConsumeGauge(UAbilitySystemComponent& ASC) const;

	UTexture2D* GetGaugeIcon() const { return GaugeIcon; }
	FLinearColor GetGaugeFullColor() const { return GaugeFullColor; }

private:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	UFUNCTION()
	void OnHealProvidedCallback(float HealDone);

	// Healing the player must provide to fill the gauge; HealCharge is capped there — extra healing is discarded.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "1.0"))
	float HealRequiredForFullCharge = 300.f;

	// Damage-multiplier boost added to the charge beam's sweet-spot multiplier when the full-gauge release lands at the
	// sweet-spot window edge (EdgeDamageMultiplierBoost) versus dead center (CenterDamageMultiplierBoost); lerped
	// between them by release precision.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float EdgeDamageMultiplierBoost = 13.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float CenterDamageMultiplierBoost = 15.f;

	// Icon shown in the HUD status bar; fills with the gauge in its own colors and shines GaugeFullColor when full.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> GaugeIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FLinearColor GaugeFullColor = FLinearColor(1.f, 0.85f, 0.2f);
};
