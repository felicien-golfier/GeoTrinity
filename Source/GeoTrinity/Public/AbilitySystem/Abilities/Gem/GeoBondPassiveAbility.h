// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"

#include "GeoBondPassiveAbility.generated.h"

/**
 * Passive of the Bond Core: the holder absorbs SharedDamageFraction of the damage each other living player takes.
 * Like the Martyr's Wrath mark, the share is taken from the hit before shield and health (AGeoGameState::
 * OnIncomingDamage), and what the holder takes carries bDoNotRedirectSacrifice, so a share is never shared or redirected
 * again: two holders hurt each other once.
 */
UCLASS()
class GEOTRINITY_API UGeoBondPassiveAbility : public UGeoCorePassiveAbility
{
	GENERATED_BODY()

protected:
	/** Server: listens to AGeoGameState::OnIncomingDamage. */
	virtual void BindEvent(UGeoAbilitySystemComponent& HolderASC) override;
	virtual void UnbindEvent(UGeoAbilitySystemComponent& HolderASC) override;

private:
	/**
	 * Makes the holder take their share of Damage when VictimASC is another living and vulnerable player, and adds it to
	 * SparedDamage. Damage is already reduced by the victim's DamageReduction; a share is worth the same before
	 * reductions: the victim's one is undone, unless the hit skipped stat modifiers, then the holder's own
	 * DamageReduction applies.
	 */
	void ShareIncomingDamage(UAbilitySystemComponent& VictimASC, FGameplayEffectContextHandle const& DamageContext,
							 float Damage, float& SparedDamage);

	/** Part of the damage an ally takes that each holder takes instead; holders can die from it. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Bond", meta = (ClampMin = "0", ClampMax = "1"))
	float SharedDamageFraction = 0.15f;
};
