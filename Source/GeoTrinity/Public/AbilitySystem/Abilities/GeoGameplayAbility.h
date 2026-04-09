// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoGameplayAbility.generated.h"


struct FGeoAbilityTargetData;
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
	virtual void InitFireSectionIndex(UAnimInstance* AnimInstance, int32& FireSectionIndex);
	void HandleAnimationMontage(UAnimInstance* AnimInstance, FGameplayAbilityActivationInfo const& ActivationInfo);
	void SendFireDataToServer(FGeoAbilityTargetData const& AbilityTargetData) const;
	virtual FGeoAbilityTargetData BuildAbilityTargetData();
	FVector GetFireSocketLocation() const;
	UFUNCTION()
	void BuildDataAndFire();

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData);

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag);


public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> AnimMontage;

	// We consider the ability to Fire at the end of the FireDuration delay.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation",
			  meta = (ClampMin = "0.01", UIMin = "0.05"))
	float FireDuration = 0.5f;

protected:
	FAbilityPayload StoredPayload;
	bool bCommitAtActivate = true;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TInstancedStruct<FEffectData>> EffectDataInstances;
	FTimerHandle FireTriggerTimerHandle;
};
