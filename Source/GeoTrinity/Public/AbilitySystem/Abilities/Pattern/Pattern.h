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

public:
	virtual void OnCreate(FGameplayTag AbilityTag);
	virtual void InitPattern(FAbilityPayload const& Payload);
	bool IsPatternActive() const { return bPatternIsActive; }

	// Must be called at the end of your pattern to call the Montage End.
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

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UTickablePattern : public UPattern
{
	GENERATED_BODY()

public:
	virtual void EndPattern() override;

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

	FTimerHandle TimeSyncTimerHandle;
};
