// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
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
	/** Overrides the base to show the charge-beam gauge (ChargeBeamGaugeComponent) instead of the deploy gauge. */
	virtual void SetChargeGaugeVisible(APlayableCharacter* Character, bool bVisible) override;

	/** Encodes the current charge ratio (0–1) into the Seed field as an integer permillage (0–1000) of the target data.
	 */
	virtual FGeoAbilityTargetData GetUpdatedTargetData() override;

	/**
	 * Appends a FContextDamageMultiplierEffectData entry to the base effect array.
	 * Non-sweet-spot: multiplier lerped from MinDamageMultiplier to MaxDamageMultiplier by charge ratio.
	 * Sweet-spot: SweetSpotDamageMultiplier used instead; when the sweet-spot charge passive's gauge is full, the
	 * passive's GetHealsToDamageMultiplier boost (lerped by GetSweetSpotPrecision) is added on top of it.
	 */
	virtual TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const override;

	/** Decodes the charge ratio (0–1) carried in StoredPayload.Seed as an integer permillage. */
	float GetStoredChargeRatio() const;

	/** Returns true when the stored charge ratio falls within the sweet-spot window. */
	bool IsSweetSpotRelease() const;

	/** Returns how close the stored charge ratio is to the sweet-spot center: 1 at dead center, 0 at the window edges.
	 */
	float GetSweetSpotPrecision() const;

	/** Fires FireGameplayCueTag on the locally-controlled client, encoding the beam endpoint, charge ratio, and
	 * sweet-spot flag into cue params. */
	void FireGameplayCue(FGeoAbilityTargetData const& AbilityTargetData);

	/** Calls Super (sends data to server), fires the beam cue locally, then ends the ability on the locally-controlled
	 * client. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;
	/** Iterates hostile actors within GeneralSpellDistance along the forward direction and applies effects. Called from
	 * Fire() for the locally-controlled player and from OnFireTargetDataReceived() on the server. */
	void DealDamage(FGeoAbilityTargetData const& AbilityTargetData) const;

	/**
	 * Server-side handler: updates StoredPayload from the client's replicated target data, then iterates all
	 * hostile actors within GeneralSpellDistance along the character's forward direction and applies effects directly.
	 */
	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

	// Sweet spot range (charge ratio 0–1). Releasing within this window applies SweetSpotBonusEffect.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float SweetSpotMinRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float SweetSpotMaxRatio = 0.7f;

	// Gameplay Cue fired on the client at the moment of release to trigger beam VFX/SFX.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	FGameplayTag FireGameplayCueTag;

	// Damage multiplier lerped from Min (0% charge) to Max (100% charge).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float MinDamageMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float MaxDamageMultiplier = 1.5f;

	// Damage multiplier applied when the charge ratio is within the sweet-spot window.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float SweetSpotDamageMultiplier = 2.f;
};
