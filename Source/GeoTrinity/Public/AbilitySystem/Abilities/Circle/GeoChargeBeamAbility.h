// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoCueParam.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoChargeBeamAbility.generated.h"

/**
 * Circle basic attack: hold to charge, release to fire a beam.
 * Client-authoritative: the machine controlling the player times the charge and the refire delay, and aims the beam.
 * The server bounds those claims (see OnFireTargetDataReceived) and hits what the beam crossed on that player's screen
 * (see DealDamage). The hold rides in the target data Seed and, through the charging curve, drives:
 *   - A lerped damage multiplier (MinDamageMultiplier to MaxDamageMultiplier)
 *   - A sweet spot bonus when released between SweetSpotMinRatio and SweetSpotMaxRatio
 * The Cooldown GE only supplies the refire delay's duration and is never applied: its tag would only leave the client
 * once the server's copy expired and replicated, a round trip after its own.
 */
UCLASS()
class GEOTRINITY_API UGeoChargeBeamAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

	/**
	 * Sets FireMode to ChargeForFireDelay so the beam charges on hold and fires on release, InstancedPerActor so
	 * NextAllowedShotTime carries from one shot to the next, and DoNotAutoCommit so the Cooldown GE is never applied.
	 */
	UGeoChargeBeamAbility();

public:
	/** Reload, so the sweet spot can be caught on a button press instead of on the charge input's release. */
	virtual FGameplayTag GetAlternateReleaseInputTag() const override;

	/**
	 * Gates activation on the refire delay Fire starts on NextAllowedShotTime, on the machine controlling the player
	 * only. A remote client's presses always pass on the server, since rejecting one would roll back a charge the
	 * client already predicted: OnFireTargetDataReceived paces them instead.
	 */
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Reports the refire delay left, for the HUD. */
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle,
													 FGameplayAbilityActorInfo const* ActorInfo, float& TimeRemaining,
													 float& CooldownDuration) const override;

protected:
	/** Overrides the base to show the charge-beam gauge (ChargeBeamGaugeComponent) instead of the deploy gauge. */
	virtual void SetChargeGaugeVisible(APlayableCharacter* Character, bool bVisible) override;

	/** Stores the current hold into the Seed field of the target data. */
	virtual FGeoAbilityTargetData GetUpdatedTargetData() override;

	/**
	 * Appends a FContextDamageMultiplierEffectData entry to the base effect array.
	 * Non-sweet-spot: multiplier lerped from MinDamageMultiplier to MaxDamageMultiplier by charge ratio.
	 * Sweet-spot: SweetSpotDamageMultiplier used instead; when the sweet-spot charge passive's gauge is full, the
	 * passive's GetHealsToDamageMultiplier boost (lerped by GetSweetSpotPrecision) is added on top of it.
	 */
	virtual TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const override;

	/** Stores a hold, in seconds, into StoredPayload.Seed as whole milliseconds. */
	void SetStoredHeldSeconds(float HeldSeconds);

	/** Decodes the hold, in seconds, carried in StoredPayload.Seed. */
	float GetStoredHeldSeconds() const;

	/** Returns the charge ratio (0–1) the stored hold reaches through the charging curve. */
	float GetStoredChargeRatio() const;

	/** Returns true when the stored charge ratio falls within the sweet-spot window. */
	bool IsSweetSpotRelease() const;

	/** Returns how close the stored charge ratio is to the sweet-spot center: 1 at dead center, 0 at the window edges.
	 */
	float GetSweetSpotPrecision() const;

	/** Fires FireCue on the locally-controlled client, encoding the beam endpoint, charge ratio, and
	 * sweet-spot flag into cue params. */
	void FireGameplayCue(FGeoAbilityTargetData const& AbilityTargetData);

	/**
	 * Calls Super (sends the data to the server), then on the machine controlling the player: deals damage on a host,
	 * plays the beam cue, starts the refire delay and ends the ability.
	 */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	/**
	 * Applies the beam's effects to the hostile and neutral damageable actors along StoredPayload's beam, over
	 * GeneralSpellDistance, the caster excluded, then spends a full sweet-spot gauge on a sweet-spot release. Each
	 * target is tested where the firing player's screen showed it: its pose (GeoLib::GetPoseAt) at that player's own
	 * time, minus the replication delay it reached that screen with.
	 */
	void DealDamage() const;

	/**
	 * Server-side handler for a remote client's release. Caps the hold it claims at the one the server saw plus
	 * ServerHoldTolerance, paces the shot from its activation's arrival with TryConsumeShotSlot (hold plus refire delay
	 * per shot), then deals damage. A shot off schedule ends the ability cancelled.
	 */
	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

	// Sweet spot range (charge ratio 0–1). Releasing within this window applies SweetSpotBonusEffect.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|ChargeBeam")
	float SweetSpotMinRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|ChargeBeam")
	float SweetSpotMaxRatio = 0.7f;

	// Cue fired at the moment of release for the beam VFX/SFX: endpoint in Location, aim direction in Normal, charge
	// ratio in RawMagnitude, sweet-spot release in NormalizedMagnitude.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|ChargeBeam")
	FGeoCueParam FireCue;

	// Damage multiplier lerped from Min (0% charge) to Max (100% charge).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|ChargeBeam")
	float MinDamageMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|ChargeBeam")
	float MaxDamageMultiplier = 1.5f;

	// Damage multiplier applied when the charge ratio is within the sweet-spot window.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|ChargeBeam")
	float SweetSpotDamageMultiplier = 2.f;
};
