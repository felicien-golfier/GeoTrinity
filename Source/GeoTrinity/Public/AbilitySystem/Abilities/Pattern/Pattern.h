// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once
#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "Pattern.generated.h"

struct FGameplayTagContainer;
class UEffectDataAsset;
class UGameplayEffect;
class AGeoProjectile;
struct FAbilityPayload;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatternEvent);

/**
 * Base class for all enemy bullet patterns. A pattern is a UObject created per-client by UGeoAbilitySystemComponent
 * in response to a multicast RPC, so it runs identically on every machine. Subclasses override StartPattern or
 * TickPattern to define when and how projectiles are spawned.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UPattern : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Called immediately after the pattern is created. AbilityTag is stored for montage section lookup.
	 * Owner is the enemy character that activated the ability; subclasses may access its components here.
	 */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner);
	/** Client-only. Fires GameplayCueTag at the pattern's zone location(s) via the instigator's ASC. Override to fire
	 * at multiple locations (e.g. one cue per pillar spawn point). */
	virtual void ExecuteGameplayCue(FGameplayTag GameplayCueTag);
	/** Builds the FGameplayCueParameters for this pattern's start cue. Override to inject custom fields (location,
	 * magnitude, etc.). */
	virtual FGameplayCueParameters FillCueParam(FAbilityPayload const& Payload);

	/**
	 * Stores the payload and triggers the start-section animation before delegating to StartPattern.
	 * PatternData carries optional pattern-specific replicated data; subclasses read their own FPatternData subclass
	 * via PatternData.GetPtr<T>(). Unset for patterns that need no extra data.
	 */
	virtual void InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData);

	/** Returns true while the pattern is running (after InitPattern, before EndPattern). */
	bool IsPatternActive() const { return bPatternIsActive; }

	/**
	 * Ends the pattern and cleans up timers.
	 * When bForceStop is false, jumps the montage to its end section and broadcasts OnPatternEnd.
	 * When bForceStop is true, stops all montages immediately and skips the OnPatternEnd broadcast —
	 * used by PatternAbility::EndAbility to force-end a pattern without re-triggering the ability end chain.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pattern")
	virtual void EndPattern(bool bForceStop = false);

	UPROPERTY(BlueprintAssignable)
	FOnPatternEvent OnPatternEnd;
	UPROPERTY(BlueprintAssignable)
	FOnPatternEvent OnPatternStart;

protected:
	// Called when montage start is done and starts the loop.
	UFUNCTION()
	virtual void StartPattern();

	void JumpMontageToEndSection() const;
	float CalculateElapsedTime() const;

	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	UPROPERTY(Transient, BlueprintReadOnly)
	FAbilityPayload StoredPayload;

	// Pattern-specific replicated data set by the launching UPatternAbility; unset when the pattern needs none.
	// Read your own FPatternData subclass via StoredPatternData.GetPtr<T>().
	UPROPERTY(Transient)
	TInstancedStruct<FPatternData> StoredPatternData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> AnimMontage;

	float StartDelay = 0.f;
	float TravelTime = 0.f;

	bool bPatternIsActive = false;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (AllowPrivateAccess = "true"))
	FGameplayTag InitGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (AllowPrivateAccess = "true"))
	FGameplayTag StartGameplayCueTag;

	FTimerHandle StartSectionTimerHandle;
};

/**
 * Pattern subclass that drives projectile spawning via a recurring timer tick.
 * Uses server-synchronized time so projectile positions are deterministic across all clients despite ping.
 * Subclasses implement TickPattern to define per-tick spawn logic.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UTickablePattern : public UPattern
{
	GENERATED_BODY()

public:
	/** Stops the tick timer, then delegates to UPattern::EndPattern. */
	virtual void EndPattern(bool bForceStop = false) override;

protected:
	virtual void StartPattern() override;
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Timer callback: reads current server time, computes SpentTime, and delegates to TickPattern. */
	UFUNCTION()
	void CalculateTimeAndTickPattern();

	/**
	 * Called each timer tick to spawn projectiles. DeltaTime is intentionally not provided — all
	 * timing must be derived from SpentTime so the pattern is deterministic across clients.
	 *
	 * @param ServerTime  Synchronized server time (replicated server time minus half ping).
	 * @param SpentTime   ServerTime minus the payload's ServerSpawnTime — elapsed time since pattern start.
	 */
	virtual void TickPattern(float ServerTime, float SpentTime);

	FTimerHandle TimeSyncTimerHandle;
};
