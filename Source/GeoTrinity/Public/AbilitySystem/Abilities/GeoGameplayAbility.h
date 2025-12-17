// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
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
	FAbilityPayload CreatePatternPayload(const FTransform& Transform, AActor* Owner, AActor* Instigator) const;
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponentFromActorInfo() const;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	TArray<struct FEffectData> GetEffectDataArray() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
	FGameplayTag StartupInputTag;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Status")
	FGameplayTag StatusToInflictTag{};

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Status", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 ChanceToInflictStatus{0};

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Pattern")
	TSubclassOf<UPattern> PatternToLaunch;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TInstancedStruct<FEffectData>> EffectDataInstances;
};
