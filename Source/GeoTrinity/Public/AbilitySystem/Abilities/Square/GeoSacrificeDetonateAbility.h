// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Data/FX/GeoCueParam.h"
#include "CoreMinimal.h"

#include "GeoSacrificeDetonateAbility.generated.h"

/**
 * Square special ability, phase 2 of the sacrifice pair (phase 1 = UGeoSacrificeBeamAbility, same button):
 * instant forward ray dealing BaseDamage + the CharacterAttributeSet's SacrificeValue to every enemy on it,
 * then resets SacrificeValue and removes the Status.Square.DetonateReady effect.
 * BP config: ActivationRequiredTags = Status.Square.DetonateReady (armed by the channel),
 * CancelAbilitiesWithTag = the channel's spell tag (a press during the channel detonates), no cooldown —
 * the channel owns the pair's cooldown.
 */
UCLASS()
class GEOTRINITY_API UGeoSacrificeDetonateAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Sets bActivateOnFreshPressOnly so a held button never chain-activates the detonation while the sacrifice channel is still active on the same input. */
	UGeoSacrificeDetonateAbility();

	/** Ready while armed (Status.Square.DetonateReady on ASC), whatever the sacrificed value. */
	virtual bool IsReady(UAbilitySystemComponent const& ASC) const override;
	/** The Square's class gauge: SacrificeValue over SacrificeForFullGauge, ready while armed (IsReady) at any fill. */
	virtual bool GetClassGauge(UAbilitySystemComponent const& ASC, float& OutFill, bool& bOutReady) const override;
	/** SacrificeValue rounded — the damage absorbed so far — while above 0; nothing at 0. */
	virtual bool GetGaugeLabel(UAbilitySystemComponent const& ASC, FText& OutLabel) const override;

protected:
	/** Local machine: fires the ray cue (damage inside is server-gated) and ends. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;
	/** Server: detonates with the client's fresh target data and ends. */
	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

private:
	/** Damages enemies on the ray and consumes the armed sacrifice (server); fires the cue (local). */
	void Detonate(FGeoAbilityTargetData const& AbilityTargetData);

	/** Detonation ray half-width in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility", meta = (ClampMin = "0", AllowPrivateAccess = true))
	float LineHalfWidth = 100.f;

	/** Flat damage added to the consumed SacrificeValue. Scales with ability level. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat BaseDamage;

	/** Fraction of consumed SacrificeValue added to the ray's total damage (total = BaseDamage + SacrificeValue × this). */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Effects", meta = (AllowPrivateAccess = true))
	float SacrificeValueMultiplier = .7f;

	/** Sacrificed damage that fills the Square's class gauge. Fill only: readiness is the armed state. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|HUD", meta = (ClampMin = "1", AllowPrivateAccess = true))
	float SacrificeForFullGauge = 200.f;

	/** Local cue fired on detonation (endpoint in Location, aim direction in Normal, value in RawMagnitude). */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|GameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam FireCue;
};
