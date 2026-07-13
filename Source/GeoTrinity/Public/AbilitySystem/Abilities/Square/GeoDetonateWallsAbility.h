// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoDetonateWallsAbility.generated.h"

/**
 * Fires a ray in front of the character (like the charge beam). The ray first counts the player's own walls
 * on its path — each wall multiplies the ray's output (WallBoostMultiplier per wall) and is consumed (recalled,
 * no explosion). The boosted ray then instantly deals damage to enemies and grants shield to allies along its path.
 */
UCLASS()
class GEOTRINITY_API UGeoDetonateWallsAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	/** Host path: applies the ray locally; remote clients send target data to the server. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;
	/** Server path for remote clients: applies the ray using the replicated target data. */
	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

private:
	/**
	 * Scans the ray, consumes (recalls) the player's own walls on it to build the multiplier, then — on the server —
	 * damages enemies and shields allies along the ray. Finally, on the locally-controlled client, fires the ray cue
	 * (scaled by the consumed wall count). Called from both Fire (host/client) and OnFireTargetDataReceived (server),
	 * so the gameplay (server-gated) and the cue (local-gated) run from the same code.
	 */
	void FireRay(FGeoAbilityTargetData const& AbilityTargetData) const;

	// Per-wall fraction of base output added by each consumed wall (N walls → WallBoostMultiplier * N); no walls means no ray.
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Detonate", meta = (AllowPrivateAccess = true))
	float WallBoostMultiplier = .5f;

	// Half-width (cm) of the ray.
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Detonate", meta = (AllowPrivateAccess = true))
	float LineHalfWidth = 15.f;

	// Base damage dealt to each enemy on the ray (before wall multiplier); scales with ability level.
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat BaseDamage;

	// Base shield granted to each ally on the ray (before wall multiplier); scales with ability level.
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat BaseShield;

	// Cue fired locally on the casting client for the ray VFX/SFX. RawMagnitude carries the consumed wall count.
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Detonate", meta = (AllowPrivateAccess = true))
	FGameplayTag FireGameplayCueTag;
};
