#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
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
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext) const;
	virtual void ApplyEffect(const FGameplayEffectContextHandle& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
		UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const;
};

UCLASS()
class UDamageEffectData : public UEffectDataAsset
{
	GENERATED_BODY()

public:
	virtual void UpdateContextHandle(FGeoGameplayEffectContext* EffectContext) const override;
	virtual void ApplyEffect(const FGameplayEffectContextHandle& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
		UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FScalableFloat DamageAmount;
};

UCLASS()
class UStatusEffectData : public UEffectDataAsset
{
	GENERATED_BODY()

public:
	virtual void UpdateContextHandle(FGeoGameplayEffectContext*) const override;
	virtual void ApplyEffect(const FGameplayEffectContextHandle& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
		UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const override;

	UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"))
	uint8 StatusChance = 0;
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag StatusTag{};
};