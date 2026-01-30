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
	FAbilityPayload CreateAbilityPayload(AActor* Owner, AActor* Instigator, FVector2D const& Origin, float Yaw,
										 float ServerSpawnTime, int Seed) const;
	FAbilityPayload CreateAbilityPayload(AActor* Owner, AActor* Instigator, FTransform const& Transform) const;
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponentFromActorInfo() const;
	TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const;
	float GetCooldown(int32 level = 1) const;
	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

protected:
	void HandleAnimationMontage(UAnimInstance const* AnimInstance,
								FGameplayAbilityActivationInfo const& ActivationInfo);
	UFUNCTION()
	virtual void AnimTrigger();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
	FGameplayTag StartupInputTag;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> AnimMontage;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TInstancedStruct<FEffectData>> EffectDataInstances;

	FTimerHandle StartSectionTimerHandle;
};
