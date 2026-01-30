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

UCLASS(BlueprintType)

class GEOTRINITY_API UEffectDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TInstancedStruct<struct FEffectData>> EffectDataInstances;
};

USTRUCT(BlueprintType)
struct GEOTRINITY_API FEffectData
{
	GENERATED_BODY()
	virtual ~FEffectData() = default;

	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext) const;
	virtual void ApplyEffect(FGameplayEffectContextHandle const& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
							 UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const;
};

USTRUCT(BlueprintType)
struct FDamageEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext) const override;
	virtual void ApplyEffect(FGameplayEffectContextHandle const& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
							 UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat DamageAmount;
};

USTRUCT(BlueprintType)
struct FStatusEffectData : public FEffectData
{
	GENERATED_BODY()

	virtual void UpdateContextHandle(FGeoGameplayEffectContext*) const override;
	virtual void ApplyEffect(FGameplayEffectContextHandle const& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
							 UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
			  meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"))
	uint8 StatusChance = 100;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag StatusTag{};
};
