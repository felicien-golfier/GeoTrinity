// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Class.h"

#include "EffectData.generated.h"

struct FGameplayEffectContextHandle;
struct FGeoGameplayEffectContext;
class UGeoAbilitySystemComponent;
class UGameplayEffect;

/**
 * Data asset that holds a reusable array of FEffectData entries.
 * Create one when the same set of effects needs to be shared across multiple abilities.
 * For effects that are specific to a single ability, use the ability's inline EffectDataInstances instead.
 */
UCLASS(BlueprintType)
class GEOTRINITY_API UEffectDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TInstancedStruct<struct FEffectData>> EffectDataInstances;
};

/**
 * Polymorphic base for all gameplay effect descriptors.
 * Used with TInstancedStruct so arrays of mixed effect types can be stored in a UPROPERTY.
 * Subclasses override UpdateContextHandle to write extra data into the context before application,
 * and ApplyEffect to perform the actual GE application.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FEffectData
{
	GENERATED_BODY()
	virtual ~FEffectData() = default;

	/**
	 * Pre-application hook: writes subclass-specific data into the effect context before any ApplyEffect call.
	 * Called in the first pass of UGeoAbilitySystemLibrary::ApplyEffectFromEffectData.
	 *
	 * @param AbilityTag  Tag of the ability that triggered this apply (invalid for non-ability sources like zones).
	 *                    Subclasses may look up the ability CDO via GetAbilityCDO to branch on its owned tags.
	 */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
									 FGameplayTag AbilityTag) const;

	/**
	 * Applies the gameplay effect described by this struct to TargetASC.
	 *
	 * @param ContextHandle  Pre-built context (may have been mutated by UpdateContextHandle).
	 * @param SourceASC      ASC of the instigator.
	 * @param TargetASC      ASC of the recipient.
	 * @param AbilityLevel   Level passed to the GE spec for scaling.
	 * @param Seed           RNG seed for deterministic randomness.
	 * @return               Handle to the applied active effect, or an invalid handle on failure.
	 */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const;
};

/** Applies an arbitrary UGameplayEffect with optional SetByCaller magnitude and duration. General-purpose. */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGameplayEffectData : public FEffectData
{
	GENERATED_BODY()
	/** Applies GameplayEffect with SetByCaller magnitude and duration; replaces any existing active instance first when
	 * bReplaceExistingInstance is set. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag DataTag;

	// Will set the Magnitude of the GE SetByCaller with given SetByCallerDataTag tag.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat Magnitude;

	// Will set the Duration magnitude of the GE SetByCaller with Data.DurationMagnitude tag.
	// If the GE is Instant or infinite, it's not used.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat Duration;

	/** When set, the HUD status bar shows this icon (texture or material) on the target while the effect is active.
	 * Carried to the client through FGeoGameplayEffectContext::Icon on this effect's spec only (the shared apply
	 * context is not touched). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
			  meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TObjectPtr<UObject> Icon;

	/** When true, an existing active instance of GameplayEffect from the same source on the target is removed before
	 * applying the new spec, so reapplication refreshes duration and magnitude instead of stacking. GE stacking
	 * settings alone only refresh the duration timer, not the SetByCaller magnitude, so this is a full replace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bReplaceExistingInstance = false;
};

/** Applies a flat damage amount. DamageAmount is evaluated at the given ability level. */
USTRUCT(BlueprintType)
struct FDamageEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Flags the context as bIsFromBasicAbility when the source ability carries the Ability.Type.Basic asset tag. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
									 FGameplayTag AbilityTag) const override;
	/** Applies the DamageEffect GE; propagates bSuppressGameplayCue, bLimitGameplayCue, bSuppressCombatStats, and
	 * bDoNotRedirectSacrifice flags onto the context. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat DamageAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIsDamagePerSecond{false};

	/** When true, unconditionally suppresses the GameplayCue embedded in the DamageEffect GE. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bSuppressGameplayCue{false};

	/** When true, UExecCalc_Damage rate-limits the GameplayCue via the target's UGeoGameFeelComponent. Use on
	 * tick-based effects. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bLimitGameplayCue{false};

	/** When true, the damage is not reported to the DPS meter (UGeoCombatStatsSubsystem). Use for self-inflicted
	 * drains. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bSuppressCombatStats{false};

	/** When true, this damage is never captured by a sacrificed receiver (redirected shares, drains, ...). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bDoNotRedirectSacrifice{false};
};

/** Applies a flat heal amount. Sets bSuppressHealProvided on the context when configured. */
USTRUCT(BlueprintType)
struct FHealEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Sets bSuppressHealProvided on the context when configured, so ExecCalc_Heal skips the OnHealProvided broadcast
	 * on the source ASC. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
									 FGameplayTag AbilityTag) const override;
	/** Applies the HealthEffect GE; propagates bSuppressGameplayCue, bLimitGameplayCue, and bSuppressCombatStats flags
	 * onto the context. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat HealAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIsHealPerSecond{false};

	// When true, the heal will not broadcast OnHealProvided on the source ASC.
	// Set on the context in UpdateContextHandle; baked into the spec via Duplicate() at MakeOutgoingSpec time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bSuppressHealProvided{false};

	/** When true, unconditionally suppresses the GameplayCue embedded in the HealthEffect GE. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bSuppressGameplayCue{false};

	/** When true, UExecCalc_Heal rate-limits the GameplayCue via the target's UGeoGameFeelComponent. Use on tick-based
	 * effects. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bLimitGameplayCue{false};

	/** When true, the heal is not reported to the HPS meter (UGeoCombatStatsSubsystem). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bSuppressCombatStats{false};
};

/** Applies a flat shield amount to the target. */
USTRUCT(BlueprintType)
struct FShieldEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Applies the ShieldEffect GE with ShieldAmount as the SetByCaller magnitude, scaled by AbilityLevel. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat ShieldAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIsShieldPerSecond{false};
};

/**
 * Sets SingleUseDamageMultiplier on the effect context for the current ApplyEffectFromEffectData call.
 * The multiplier is consumed by UExecCalc_Damage and automatically resets on the next call (fresh context).
 * Append this entry to an effect array to scale damage for that specific apply call only.
 */
USTRUCT(BlueprintType)
struct FContextDamageMultiplierEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Sets SingleUseDamageMultiplier on the context to Multiplier, scaling the next damage application in the same
	 * apply call. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
									 FGameplayTag AbilityTag) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	FScalableFloat Multiplier{2.f};
};

/** Instantly kills the target by applying the LethalEffect from UGameDataSettings. */
USTRUCT(BlueprintType)
struct FLethalEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Applies GameDataSettings::LethalEffect to the target, setting its health to zero. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;
};

/**
 * Probabilistically applies a status effect (debuff) to the target.
 * StatusChance (0–100) is rolled on each apply; on failure nothing happens.
 */
USTRUCT(BlueprintType)
struct FStatusEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Rolls StatusChance (0–100) against a seeded random and applies the GE for StatusTag only on success. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
			  meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"))
	uint8 StatusChance = 100;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag StatusTag{};
};
