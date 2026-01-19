// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoGameplayAbility.generated.h"

struct FAbilityPayload;
class UEffectDataAsset;
class UGeoAbilitySystemComponent;
class UPattern;

/**
 *
 */
UCLASS()
class GEOTRINITY_API UGeoGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	FGameplayTag GetAbilityTag() const;
	FAbilityPayload CreateAbilityPayload(const FTransform& Transform, AActor* Owner, AActor* Instigator) const;
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponentFromActorInfo() const;
	TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const;
	float GetCooldown(int32 level = 1) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
	FGameplayTag StartupInputTag;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TInstancedStruct<FEffectData>> EffectDataInstances;
};
