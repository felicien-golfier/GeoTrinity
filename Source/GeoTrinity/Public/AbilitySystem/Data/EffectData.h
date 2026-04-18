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
 * Data asset that holds a reusable collection of polymorphic FEffectData entries.
 * Assign to ability blueprints when effects are shared across multiple abilities.
 * For effects specific to a single ability, use the inline TArray on the ability instead.
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
 * Generic FEffectData that applies a configurable UGameplayEffect with optional SetByCaller magnitude and duration.
 * The default choice for simple effects that do not require custom ExecCalc logic.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGameplayEffectData : public FEffectData
{
	GENERATED_BODY()
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
 * Applies the global damage gameplay effect via ExecCalc_Damage.
 * DamageAmount is passed through the context; multipliers set by FContextDamageMultiplierEffectData are applied inside ExecCalc.
 */
USTRUCT(BlueprintType)
struct FDamageEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat DamageAmount;
};

/**
 * Applies the global heal gameplay effect.
 * Optionally suppresses the OnHealProvided broadcast so heal-return passives (e.g. Circle's self-heal) are not triggered.
 */
USTRUCT(BlueprintType)
struct FHealEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel) const override;
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

/** Applies the global shield gameplay effect with a configurable shield amount. */
USTRUCT(BlueprintType)
struct FShieldEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual FActiveGameplayEffectHandle ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat ShieldAmount;
};

/**
 * Writes a one-shot damage multiplier into the effect context before the damage GE is applied.
 * Consumed by ExecCalc_Damage; only affects the FDamageEffectData in the same ApplyEffectFromEffectData call.
 * Append before FDamageEffectData in the effect array — order within a call does not matter due to two-pass execution.
 */
USTRUCT(BlueprintType)
struct FContextDamageMultiplierEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	FScalableFloat Multiplier{2.f};
};

/**
 * Applies a status effect (buff or debuff) identified by StatusTag, with a configurable application chance.
 * StatusChance = 100 always applies; lower values roll against the chance each time.
 */
USTRUCT(BlueprintType)
struct FStatusEffectData : public FEffectData
{
	GENERATED_BODY()

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
