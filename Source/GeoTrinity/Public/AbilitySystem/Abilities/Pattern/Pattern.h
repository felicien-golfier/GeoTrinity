// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once
#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "Pattern.generated.h"

struct FGameplayTagContainer;
class UEffectDataAsset;
class UGameplayEffect;
class AGeoProjectile;
struct FAbilityPayload;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatternEnd);

/**
 * Base class for all enemy bullet patterns. A pattern is a UObject created per-client by UGeoAbilitySystemComponent
 * in response to a multicast RPC, so it runs identically on every machine. Subclasses override StartPattern or
 * TickPattern to define when and how projectiles are spawned.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UPattern : public UObject
{
	GENERATED_BODY()

	UFUNCTION()
	void OnMontageSectionStartEnded();

public:
	/** Called immediately after the pattern is created. AbilityTag is stored for montage section lookup. */
	virtual void OnCreate(FGameplayTag AbilityTag);

	/** Stores the payload and triggers the start-section animation before delegating to StartPattern. */
	virtual void InitPattern(FAbilityPayload const& Payload);

	/** Returns true while the pattern is running (after InitPattern, before EndPattern). */
	bool IsPatternActive() const { return bPatternIsActive; }

	/** Ends the pattern and jumps the animation montage to its end section. Must be called to clean up. */
	UFUNCTION(BlueprintCallable, Category = "Pattern")
	virtual void EndPattern();

	UPROPERTY(BlueprintAssignable)
	FOnPatternEnd OnPatternEnd;

protected:
	// Called when montage start is done and starts the loop.
	virtual void StartPattern(FAbilityPayload const& Payload);

	UFUNCTION(BlueprintNativeEvent, Category = "Pattern")
	void OnStartPattern(FAbilityPayload const& Payload);

	void JumpMontageToEndSection() const;

	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	float StartSectionLength = 0.f;

	FAbilityPayload StoredPayload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AnimMontage;

private:
	bool bPatternIsActive = false;
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
	virtual void EndPattern() override;

protected:
	virtual void StartPattern(FAbilityPayload const& Payload) override;
	virtual void InitPattern(FAbilityPayload const& Payload) override;
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
