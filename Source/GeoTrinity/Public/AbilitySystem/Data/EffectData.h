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
 * Data asset container for a reusable list of FEffectData instances.
 * Use this when the same set of effects must be shared across multiple abilities.
 * For effects specific to one ability, prefer inline TArray<TInstancedStruct<FEffectData>> on the ability directly.
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
	 */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel) const;

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

/**
 * Applies an arbitrary UGameplayEffect to the target.
 * Supports optional SetByCaller magnitude (DataTag) and duration magnitude.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGameplayEffectData : public FEffectData
{
	GENERATED_BODY()
	/** Applies GameplayEffect at the given ability level, optionally setting SetByCaller magnitudes from Magnitude and Duration. */
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
};

/**
 * Applies raw damage to the target.
 * ExecCalc_Damage resolves attribute modifiers, crits, and the SingleUseDamageMultiplier from the context.
 */
USTRUCT(BlueprintType)
struct FDamageEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Applies the damage GE scaled by DamageAmount. Crit and multiplier handling is delegated to ExecCalc_Damage. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat DamageAmount;
};

/**
 * Applies a heal to the target.
 * If bSuppressHealProvided is true, the heal does not broadcast OnHealProvided — useful to prevent the
 * Circle passive's heal-return loop from triggering on self-heals.
 */
USTRUCT(BlueprintType)
struct FHealEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Writes bSuppressHealProvided into the context so the flag survives the MakeOutgoingSpec Duplicate pass. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel) const override;
	/** Applies the heal GE scaled by HealAmount. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat HealAmount;

	// When true, the heal will not broadcast OnHealProvided on the source ASC.
	// Set on the context in UpdateContextHandle; baked into the spec via Duplicate() at MakeOutgoingSpec time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bSuppressHealProvided{false};
};

/**
 * Applies a shield to the target, absorbing incoming damage up to ShieldAmount.
 */
USTRUCT(BlueprintType)
struct FShieldEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Applies the shield GE scaled by ShieldAmount. */
	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat ShieldAmount;
};

/**
 * Injects a one-shot damage multiplier into the effect context.
 * No GE is applied — this entry only mutates SingleUseDamageMultiplier on the shared context.
 * ExecCalc_Damage reads and applies it during the same ApplyEffectFromEffectData call.
 * @note Append to the effect array alongside a FDamageEffectData to scale that specific apply call.
 */
USTRUCT(BlueprintType)
struct FContextDamageMultiplierEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Writes Multiplier into FGeoGameplayEffectContext::SingleUseDamageMultiplier. */
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	FScalableFloat Multiplier{2.f};
};

/**
 * Applies a status effect (debuff) with a configurable proc chance.
 * A random roll is compared against StatusChance (0–100); the GE is skipped when the roll exceeds it.
 */
USTRUCT(BlueprintType)
struct FStatusEffectData : public FEffectData
{
	GENERATED_BODY()

	/** Applies the status GE if a random roll (0–100) falls within StatusChance. */
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
