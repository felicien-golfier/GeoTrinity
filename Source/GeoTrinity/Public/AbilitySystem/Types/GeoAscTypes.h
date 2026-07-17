// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"

#include "GeoAscTypes.generated.h"

class UGameplayEffect;

/**
 * Extended gameplay effect context that carries GeoTrinity-specific per-hit data: the applied StatusTag, the replicated
 * status-bar Icon, and call-site-scoped fields (SingleUseDamageMultiplier, bSuppressHealProvided) that are embedded into
 * the spec via Duplicate() so they survive the MakeOutgoingSpec copy.
 */
USTRUCT(BlueprintType)
struct FGeoGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	/** Returns the tag of the status effect applied alongside this hit (invalid tag when none). */
	FGameplayTag GetStatusTag() const { return StatusTag; }
	/** Returns the call-site damage multiplier for the current ExecCalc_Damage evaluation. */
	float GetSingleUseDamageMultiplier() const { return SingleUseDamageMultiplier; }
	/** Returns true when OnHealProvided is suppressed on the source ASC for this context. */
	bool IsSuppressHealProvided() const { return bSuppressHealProvided; }
	/** Returns true when the GameplayCue embedded in the applied effect should be skipped unconditionally. */
	bool IsSuppressGameplayCue() const { return bSuppressGameplayCue; }
	/** Returns true when the GameplayCue should be rate-limited via the target's UGeoGameFeelComponent in ExecCalc. */
	bool IsLimitGameplayCue() const { return bLimitGameplayCue; }
	/** Returns true when the damage/heal should not be reported to UGeoCombatStatsSubsystem. */
	bool IsSuppressCombatStats() const { return bSuppressCombatStats; }
	/** Returns true when this damage originates from a basic ability; ExecCalc records the target on the source ASC. */
	bool IsFromBasicAbility() const { return bIsFromBasicAbility; }
	/** Returns true when this damage must never be captured by a sacrificed target (redirected shares, drains, ...). */
	bool DoNotRedirectSacrifice() const { return bDoNotRedirectSacrifice; }
	/** Icon (UTexture2D or UMaterialInterface) shown in the HUD status bar while the applied effect is active;
	 * null when the effect has no icon. */
	UObject* GetIcon() const { return Icon; }

	/** Records the tag of the status effect applied alongside this hit. */
	void SetStatusTag(FGameplayTag statusTag) { StatusTag = statusTag; }
	/** Sets the call-site damage multiplier consumed by ExecCalc_Damage for the current apply call. */
	void SetSingleUseDamageMultiplier(float value) { SingleUseDamageMultiplier = value; }
	/** When true, OnHealProvided is not broadcast on the source ASC (suppressed in ExecCalc_Heal). */
	void SetSuppressHealProvided(bool value) { bSuppressHealProvided = value; }
	/** When true, the GameplayCue embedded in the applied effect will be suppressed unconditionally. */
	void SetSuppressGameplayCue(bool value) { bSuppressGameplayCue = value; }
	/** When true, ExecCalc rate-limits the GameplayCue via the target's UGeoGameFeelComponent. Use on tick-based effects. */
	void SetLimitGameplayCue(bool value) { bLimitGameplayCue = value; }
	/** When true, skips reporting damage/heal to the DPS/HPS meter in UGeoCombatStatsSubsystem. */
	void SetSuppressCombatStats(bool value) { bSuppressCombatStats = value; }
	/** When true, ExecCalc records the hit target as the source ASC's last basic-ability target (for turret retargeting). */
	void SetIsFromBasicAbility(bool value) { bIsFromBasicAbility = value; }
	/** When true, PostGameplayEffectExecute never captures this damage for sacrifice redirection. */
	void SetDoNotRedirectSacrifice(bool value) { bDoNotRedirectSacrifice = value; }
	/** Sets the icon (UTexture2D or UMaterialInterface) the HUD status bar displays while the applied effect is
	 * active. */
	void SetIcon(UObject* value) { Icon = value; }

	/** Returns the static struct type for this context; required by the GAS replication system for type identification. */
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

	/**
	 * Deep-copies the context, including call-site scoped fields (SingleUseDamageMultiplier, bSuppressHealProvided)
	 * that are not replicated, so they survive the MakeOutgoingSpec copy and reach PostGameplayEffectExecute.
	 */
	virtual FGeoGameplayEffectContext* Duplicate() const override;

	/** Serializes all extended context fields for replication. Call-site scoped fields are excluded. */
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

protected:
	UPROPERTY()
	FGameplayTag StatusTag{};
	UPROPERTY()
	TObjectPtr<UObject> Icon{nullptr};

	// Call-site scoped — set by UpdateContextHandle, baked into the spec context via Duplicate() at MakeOutgoingSpec time.
	// Not serialized: consumed server-side from the spec's embedded context copy in ExecCalc / PostGameplayEffectExecute.
	float SingleUseDamageMultiplier{1.f};
	bool bSuppressHealProvided{false};
	bool bSuppressGameplayCue{false};
	bool bLimitGameplayCue{false};
	bool bSuppressCombatStats{false};
	bool bIsFromBasicAbility{false};
	bool bDoNotRedirectSacrifice{false};
};

// ---------------------------------------------------------------------------------------------------------------------
template <>
struct TStructOpsTypeTraits<FGeoGameplayEffectContext> : TStructOpsTypeTraitsBase2<FGeoGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true // Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};
