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

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UPattern : public UObject
{
	GENERATED_BODY()

public:
	void OnCreate(FGameplayTag AbilityTag);

	UFUNCTION(BlueprintNativeEvent)
	void StartPattern(const FAbilityPayload& Payload);
	virtual void StartPattern_Implementation(const FAbilityPayload& Payload);

protected:
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> AnimMontage;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UTickablePattern : public UPattern
{
	GENERATED_BODY()

protected:
	virtual void StartPattern_Implementation(const FAbilityPayload& Payload) override;
	UFUNCTION()
	void CalculateDeltaTimeAndTickPattern();

	// We do NOT give the delta time, because we want pattern to be deterministic !
	// Use SpentTime to re-calculate every frame your stuff.
	// @param ServerTime : Gives synchronized server time, it is the replicated server time - 1/2 ping.
	// @param SpentTime : ServerTime - ServerSpawnTime
	virtual void TickPattern(float ServerTime, float SpentTime);

	// Must be called when the last tick is done. BEFORE the next Pattern of this same instance is starded again !
	virtual void EndPattern();

	FAbilityPayload StoredPayload;
	FTimerHandle TimeSyncTimerHandle;

	bool bPatternIsActive = false;
};
