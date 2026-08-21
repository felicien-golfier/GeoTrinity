// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/GeoDifficulty.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TInstancedStruct<struct FEffectData>> EffectDataInstances;
};

// Level bounds a description value evaluated as a range is rendered over — for values whose curve is driven by
// another system than ability level (e.g. the reload's remaining-ammo scale), rendered as a level 1 → 10 range.
constexpr int32 MinDescriptionLevel = 1;
constexpr int32 MaxDescriptionLevel = 10;

// How a resolved scalar value is rendered: as-is, or as a percentage (raw ×100) / bonus percentage ((raw−1)×100),
// selected per token by a {Token:%} / {Token:+%} suffix so a 1.5 multiplier reads "150%" or "50%".
enum class EValueFormat : uint8
{
	Plain,
	Percent,
	BonusPercent
};

// Per-token render options. Levels are equal unless bShowRange, in which case a value is evaluated at
// MinDescriptionLevel and MaxDescriptionLevel to show its full curve range.
struct FDescriptionFormat
{
	int32 AbilityLevel = 1;
	bool bShowRange = false;
	bool bRichTextValues = false;
	EValueFormat ValueFormat = EValueFormat::Plain;

	int32 MinLevel() const { return bShowRange ? MinDescriptionLevel : AbilityLevel; }
	int32 MaxLevel() const { return bShowRange ? MaxDescriptionLevel : AbilityLevel; }
};

/** Wraps a resolved value in the <Value> rich-text style tag so the UI can color it. */
FString MarkUpValue(FString const& Value, FDescriptionFormat const& Format);
/** Renders "Min-Max" (or a single value when both are equal) with the format's percentage suffix and mark-up. */
FString FormatValueRange(float Min, float Max, FDescriptionFormat const& Format);
/** FormatValueRange over the scalable's values at the format's min and max levels. */
FString FormatScalableRange(FScalableFloat const& Scalable, FDescriptionFormat const& Format);
/** Last segment of a tag's name ("Status.Burn" → "Burn"), used as a display label. */
FString GetTagLeafName(FGameplayTag const& Tag);

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

	/** True when the magnitude is a rate scaled on apply by the tick it covers (the frame, or the length a slower
	 * applier passed), so continuous sources (beams, zones) must re-apply it every tick a target stays inside instead
	 * of once on entry. */
	virtual bool IsPerSecond() const { return false; }

	/**
	 * One tooltip line describing this entry ("Damage: 30-50"), empty when the entry shows nothing. Every {Effects}
	 * token in an ability description is the non-empty lines of its effect array, so a new subclass only has to
	 * override this to appear in tooltips.
	 */
	virtual FString GetDescriptionLine(FDescriptionFormat const& Format) const;

	/**
	 * True when this entry applies at AbilityLevel.
	 *
	 * A level is only an EGeoDifficulty bit when a boss is the source; elsewhere it is a plain magnitude scalar (a
	 * loot pickup levels 3-10 off its power roll). Masking those would drop level 8 on the nose, so the untouched
	 * all-difficulties value short-circuits instead — narrowing the mask is what opts an entry into the gate, and
	 * only boss effects ever do.
	 */
	bool AppliesAtLevel(int32 AbilityLevel) const
	{
		return Difficulties == GeoDifficultyMask::All || (Difficulties & AbilityLevel) != 0;
	}

	/**
	 * Difficulties this entry applies at, all three by default. Unticking Safe keeps a lethal hit out of the easy
	 * tuning without a second copy of the ability.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.EGeoDifficulty"))
	int32 Difficulties = GeoDifficultyMask::All;
};

/** Applies an arbitrary UGameplayEffect with optional SetByCaller magnitude and duration. General-purpose. */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGameplayEffectData : public FEffectData
{
	GENERATED_BODY()
	/** "DataTag or GE name: Magnitude" plus " for Durations" when the GE has one. */
	virtual FString GetDescriptionLine(FDescriptionFormat const& Format) const override;

	/** Applies GameplayEffect with SetByCaller magnitude and duration; replaces any existing active instance first when
	 * bReplaceExistingInstance is set. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag DataTag;

	// Will set the Magnitude of the GE SetByCaller with given SetByCallerDataTag tag.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FScalableFloat Magnitude;

	// Will set the Duration magnitude of the GE SetByCaller with Data.DurationMagnitude tag.
	// If the GE is Instant or infinite, it's not used.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FScalableFloat Duration;

	/** When set, the HUD status bar shows this icon (texture or material) on the target while the effect is active.
	 * Carried to the client through FGeoGameplayEffectContext::Icon on this effect's spec only (the shared apply
	 * context is not touched). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TObjectPtr<UObject> Icon;

	/** When true, an existing active instance of GameplayEffect from the same source on the target is removed before
	 * applying the new spec, so reapplication refreshes duration and magnitude instead of stacking. GE stacking
	 * settings alone only refresh the duration timer, not the SetByCaller magnitude, so this is a full replace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bReplaceExistingInstance = false;
};

/**
 * A magnitude applied through one of the UGameDataSettings gameplay effects: damage, heal, shield.
 * Everything but the effect class and the SetByCaller tag is identical between them, so subclasses only supply
 * those two — GetMagnitudeTag also names the value in tooltips and is what a {Damage} / {Heal} / {Shield} token
 * sums over.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FMagnitudeEffectData : public FEffectData
{
	GENERATED_BODY()

	/** The UGameDataSettings GE this magnitude is applied through, loaded synchronously; null when unconfigured. */
	virtual TSubclassOf<UGameplayEffect> GetEffectClass() const { return nullptr; }
	/** SetByCaller tag Amount is assigned to, and the label the tooltip line uses. */
	virtual FGameplayTag GetMagnitudeTag() const { return FGameplayTag(); }

	/** Propagates bSuppressGameplayCue, bLimitGameplayCue and bSuppressCombatStats onto the context. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
									 FGameplayTag AbilityTag) const override;
	/** Applies GetEffectClass() with Amount (per second: times the context's delta) under GetMagnitudeTag(). */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	virtual bool IsPerSecond() const override { return bIsPerSecond; }
	virtual FString GetDescriptionLine(FDescriptionFormat const& Format) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FScalableFloat Amount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsPerSecond{false};

	/** When true, unconditionally suppresses the GameplayCue embedded in the applied GE. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSuppressGameplayCue{false};

	/** When true, the ExecCalc rate-limits the GameplayCue via the target's UGeoGameFeelComponent. Use on tick-based
	 * effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "!bIsPerSecond", EditConditionHides))
	bool bLimitGameplayCue{false};

	/** When true, the magnitude is not reported to the DPS/HPS meter (UGeoCombatStatsSubsystem). Use for
	 * self-inflicted drains. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSuppressCombatStats{false};
};

/** Applies a flat damage amount. Amount is evaluated at the given ability level. */
USTRUCT(BlueprintType)
struct FDamageEffectData : public FMagnitudeEffectData
{
	GENERATED_BODY()

	virtual TSubclassOf<UGameplayEffect> GetEffectClass() const override;
	virtual FGameplayTag GetMagnitudeTag() const override;

	/** Adds bDoNotRedirectSacrifice, and flags the context as bIsFromBasicAbility when the source ability carries the
	 * Ability.Type.Basic asset tag. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
									 FGameplayTag AbilityTag) const override;

	/** When true, this damage is never captured by a sacrificed receiver (redirected shares, drains, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDoNotRedirectSacrifice{false};
};

/** Applies a flat heal amount. Sets bSuppressHealProvided on the context when configured. */
USTRUCT(BlueprintType)
struct FHealEffectData : public FMagnitudeEffectData
{
	GENERATED_BODY()

	virtual TSubclassOf<UGameplayEffect> GetEffectClass() const override;
	virtual FGameplayTag GetMagnitudeTag() const override;

	/** Adds bSuppressHealProvided, so ExecCalc_Heal skips the OnHealProvided broadcast on the source ASC. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
									 FGameplayTag AbilityTag) const override;

	// When true, the heal will not broadcast OnHealProvided on the source ASC.
	// Set on the context in UpdateContextHandle; baked into the spec via Duplicate() at MakeOutgoingSpec time.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSuppressHealProvided{false};
};

/** Applies a flat shield amount to the target. */
USTRUCT(BlueprintType)
struct FShieldEffectData : public FMagnitudeEffectData
{
	GENERATED_BODY()

	virtual TSubclassOf<UGameplayEffect> GetEffectClass() const override;
	virtual FGameplayTag GetMagnitudeTag() const override;
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
	/** "X% more damage". */
	virtual FString GetDescriptionLine(FDescriptionFormat const& Format) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	FScalableFloat Multiplier{2.f};
};

/** Instantly kills the target by applying the LethalEffect from UGameDataSettings. */
USTRUCT(BlueprintType)
struct FLethalEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual FString GetDescriptionLine(FDescriptionFormat const& Format) const override;

	/** Applies GameDataSettings::LethalEffect to the target, setting its health to zero. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;
};
