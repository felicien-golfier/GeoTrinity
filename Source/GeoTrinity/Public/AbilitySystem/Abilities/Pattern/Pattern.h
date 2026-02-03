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

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UPattern : public UObject
{
	GENERATED_BODY()

	UFUNCTION()
	void OnMontageSectionStartEnded();

protected:
	// Called when montage start is done and starts the loop.
	virtual void StartPattern(FAbilityPayload const& Payload);

	UFUNCTION(BlueprintNativeEvent, Category = "Pattern")
	void OnStartPattern(FAbilityPayload const& Payload);

	void JumpMontageToEndSection() const;
	// Must be called at the end of your pattern to call the Montage End.
	UFUNCTION(BlueprintCallable, Category = "Pattern")
	virtual void EndPattern();

public:
	virtual void OnCreate(FGameplayTag AbilityTag);

	virtual void InitPattern(FAbilityPayload const& Payload);

	bool IsPatternActive() const { return bPatternIsActive; }

private:
	bool bPatternIsActive = false;
	FTimerHandle StartSectionTimerHandle;

protected:
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	float StartSectionLength = 0.f;

	FAbilityPayload StoredPayload;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> AnimMontage;

public:
	FOnPatternEnd OnPatternEnd;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UTickablePattern : public UPattern
{
	GENERATED_BODY()

protected:
	virtual void StartPattern(FAbilityPayload const& Payload) override;
	virtual void InitPattern(FAbilityPayload const& Payload) override;
	UFUNCTION()
	void CalculateTimeAndTickPattern();

	// We do NOT give the delta time because we want pattern to be deterministic !
	// Use SpentTime to re-calculate every frame your stuff.
	// @param ServerTime : Gives synchronized server time, it is the replicated server time - 1/2 ping.
	// @param SpentTime : ServerTime - ServerSpawnTime
	virtual void TickPattern(float ServerTime, float SpentTime);

	// Must be called when the last tick is done. BEFORE the next Pattern of this same instance is starded again !
	virtual void EndPattern() override;

	FTimerHandle TimeSyncTimerHandle;
};
