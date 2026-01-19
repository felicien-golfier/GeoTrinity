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
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UTickablePattern : public UPattern
{
	GENERATED_BODY()

protected:
	virtual void StartPattern_Implementation(const FAbilityPayload& Payload) override;
	UFUNCTION()
	void CalculateDeltaTimeAndTickPattern();

	virtual void TickPattern(float DeltaSeconds);

	// Must be called when the last tick is done. BEFORE the next Pattern of this same instance is starded again !
	virtual void EndPattern();

	FAbilityPayload StoredPayload;
	double PreviousFrameTime;
	FTimerHandle TimeSyncTimerHandle;

	bool bPatternIsActive = false;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UProjectilePattern : public UPattern
{
	GENERATED_BODY()

protected:
	virtual void StartPattern_Implementation(const FAbilityPayload& Payload) override;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(const FAbilityPayload& Payload, float Yaw);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;
};