// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoSweetSpotChargePassiveAbility.generated.h"

class UTexture2D;

/**
 * Passive ability for the Circle player.
 * Listens to OnHealProvided on the owner's ASC (server). The first heal after the gauge was last consumed starts a
 * charge window of ChargeDuration seconds (start stored in the HealChargeStartTime attribute); healing done during the
 * window accumulates into the HealCharge attribute while the HUD status-bar gauge fills bottom-to-top with elapsed
 * time. Once the window elapses the gauge is full (tinted GaugeFullColor) and the next charge-beam sweet-spot release
 * deals HealToDamageRatio × HealCharge damage instead of its base damage, then consumes the gauge
 * (UGeoChargeBeamAbility::DealDamage → ConsumeGauge).
 */
UCLASS()
class GEOTRINITY_API UGeoSweetSpotChargePassiveAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	UGeoSweetSpotChargePassiveAbility();

	/** Returns the granted passive on ASC, or nullptr when the owner doesn't have it (non-Circle players). */
	static UGeoSweetSpotChargePassiveAbility const* FindOnASC(UAbilitySystemComponent const& ASC);

	/** Returns the gauge fill (0–1): time elapsed since the charge window started over ChargeDuration; 0 while idle. */
	float GetGaugeRatio(UAbilitySystemComponent const& ASC) const;

	/** Returns the damage a full-gauge sweet-spot release deals: HealToDamageRatio × the recorded HealCharge. */
	float GetBoostDamage(UAbilitySystemComponent const& ASC) const;

	/** Zeroes HealCharge and HealChargeStartTime, so the next heal starts a fresh charge window. Server only. */
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

	// Duration of the charge window: starts on the first heal after consumption, gauge is full once it elapses.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.1"))
	float ChargeDuration = 8.f;

	// Fraction of the healing recorded during the charge window dealt as sweet-spot damage on a full-gauge release.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float HealToDamageRatio = 0.5f;

	// Icon shown in the HUD status bar; fills with the gauge and tints GaugeFullColor when full.
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> GaugeIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FLinearColor GaugeFullColor = FLinearColor(1.f, 0.85f, 0.2f);
};
