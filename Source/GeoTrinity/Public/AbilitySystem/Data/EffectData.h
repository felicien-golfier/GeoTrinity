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
 * Data asset that holds a reusable array of FEffectData instances.
 * Reference it from ability BlueprintReadOnly properties to share an effect configuration
 * across multiple abilities without duplicating inline EffectDataInstances.
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
 * Generic FEffectData wrapper that applies any UGameplayEffect class.
 * Magnitude is set via SetByCaller (DataTag) and Duration via Gameplay_DurationMagnitude.
 * Use for effects that are not pure damage or heal, or when the GE class must be specified per-ability.
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
 * Applies the global DamageEffect GE (configured in UGameDataSettings).
 * DamageAmount is passed via SetByCaller with Gameplay_Damage; processed by UExecCalc_Damage.
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
 * Applies the global HealthEffect GE (configured in UGameDataSettings).
 * HealAmount is passed via SetByCaller with Gameplay_Heal.
 * Set bSuppressHealProvided to prevent broadcasting OnHealProvided on the source ASC for this heal.
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

/**
 * Applies the global ShieldEffect GE (configured in UGameDataSettings).
 * ShieldAmount is passed via SetByCaller with Gameplay_Shield.
 */
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
 * Writes a one-shot damage multiplier into the effect context before any damage GE is applied in the same
 * ApplyEffectFromEffectData call. Consumed by UExecCalc_Damage via FGeoGameplayEffectContext::GetSingleUseDamageMultiplier.
 * Automatically reset for the next call because the context is rebuilt fresh each time.
 * @warning Multiplier value must not be 1 (a no-op multiplier is a configuration error).
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
 * Probabilistically applies a status tag GE to the target.
 * Whether the effect fires is determined by comparing (Seed / MAX_int32 * 100) against StatusChance.
 * StatusChance = 100 guarantees application; StatusChance = 0 guarantees no application.
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
