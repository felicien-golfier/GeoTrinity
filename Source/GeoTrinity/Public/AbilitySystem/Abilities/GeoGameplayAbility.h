// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilityPayload.h"
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
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
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
	void ScheduleFireTrigger(FGameplayAbilityActivationInfo const& ActivationInfo, UAnimInstance* AnimInstance);
	void HandleAnimationMontage(UAnimInstance* AnimInstance, FGameplayAbilityActivationInfo const& ActivationInfo);
	UFUNCTION()
	virtual void Fire();

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag);


public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
	FGameplayTag StartupInputTag;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> AnimMontage;

	// We consider the ability to Fire at the end of the FireRate delay.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation",
			  meta = (ClampMin = "0.01", UIMin = "0.05"))
	float FireRate = 0.5f;

protected:
	FAbilityPayload StoredPayload;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets;
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TInstancedStruct<FEffectData>> EffectDataInstances;

	FTimerHandle FireTriggerTimerHandle;
};
