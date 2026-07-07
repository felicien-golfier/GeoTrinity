// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
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
	UGeoSacrificeDetonateAbility();

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
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0", AllowPrivateAccess = true))
	float LineHalfWidth = 100.f;

	/** Flat damage added to the consumed SacrificeValue. Scales with ability level. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat BaseDamage;

	/** Local cue fired on detonation (endpoint in Location, aim direction in Normal, value in RawMagnitude). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag FireGameplayCueTag;
};
