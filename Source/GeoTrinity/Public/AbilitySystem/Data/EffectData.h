#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Class.h"

#include "EffectData.generated.h"

struct FGameplayEffectContextHandle;
struct FGeoGameplayEffectContext;
class UGeoAbilitySystemComponent;

UCLASS()
class GEOTRINITY_API UEffectDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Data")
	TArray<FInstancedStruct> EffectDataInstances;
};

USTRUCT(BlueprintType)
struct FEffectData
{
	GENERATED_BODY()
	virtual ~FEffectData() = default;

	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext);
	virtual void ApplyEffect(const FGameplayEffectContextHandle& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
		UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed);
};

USTRUCT(BlueprintType)
struct FDamageEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext) override;
	virtual void ApplyEffect(const FGameplayEffectContextHandle& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
		UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FScalableFloat DamageAmount;
};

USTRUCT(BlueprintType)
struct FStatusEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual void UpdateContextHandle(FGeoGameplayEffectContext*) override;
	virtual void ApplyEffect(const FGameplayEffectContextHandle& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
		UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) override;

	UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"))
	uint8 StatusChance = 0;
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag StatusTag{};
};