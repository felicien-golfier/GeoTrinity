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

	/** Encodes the current charge ratio (0–1) into the Seed field as an integer permillage (0–1000) of the target data. */
	virtual FGeoAbilityTargetData GetUpdatedTargetData() override;

	/**
	 * Appends a FContextDamageMultiplierEffectData entry to the base effect array.
	 * The multiplier is lerped from MinDamageMultiplier to MaxDamageMultiplier based on the charge ratio.
	 */
	virtual TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const override;
	/** Fires FireGameplayCueTag on the locally-controlled client, encoding the beam endpoint, charge ratio, and sweet-spot flag into cue params. */
	void FireGameplayCue(FGeoAbilityTargetData const& AbilityTargetData);

	/** Calls Super (sends data to server), fires the beam cue locally, then ends the ability on the locally-controlled client. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

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
	float MinDamageMultiplier = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|ChargeBeam")
	float MaxDamageMultiplier = 1.5f;
};
