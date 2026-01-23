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

private:
	int GetAndCheckSection(FName Section) const;

	UFUNCTION()
	void OnMontageSectionStartEnded();

protected:
	UAnimInstance* GetAnimInstance(const FAbilityPayload& Payload) const;

	// Called when montage start is done and starts the loop.
	virtual void StartPattern(const FAbilityPayload& Payload);

	UFUNCTION(BlueprintNativeEvent, Category = "Pattern")
	void OnStartPattern(const FAbilityPayload& Payload);

	void JumpMontageToEndSection() const;
	// Must be called at the end of your pattern to call the Montage End.
	UFUNCTION(BlueprintCallable, Category = "Pattern")
	virtual void EndPattern();

public:
	virtual void OnCreate(FGameplayTag AbilityTag);

	virtual void InitPattern(const FAbilityPayload& Payload);

	bool IsPatternActive() const { return bPatternIsActive; };

private:
	bool bPatternIsActive = false;
	FTimerHandle StartSectionTimerHandle;

protected:
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	float StartSectionLength = 0.f;
	inline static const FName SectionStartName{"Start"};
	inline static const FName SectionLoopName{"Loop"};
	inline static const FName SectionEndName{"End"};

	FAbilityPayload StoredPayload;

public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> AnimMontage;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UTickablePattern : public UPattern
{
	GENERATED_BODY()

protected:
	virtual void StartPattern(const FAbilityPayload& Payload) override;
	virtual void InitPattern(const FAbilityPayload& Payload) override;
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
