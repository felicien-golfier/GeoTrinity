// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoAbilitySystemLibrary.generated.h"

class UStatusInfo;
struct FDamageEffectParams;
class UAbilityInfo;
struct FGameplayAbilitySpec;
class UGameplayAbility;
struct FGameplayEffectContextHandle;
struct FGameplayTag;
class UAbilitySystemComponent;
/**
 * A library of helper functions for the ASC
 */
UCLASS()
class GEOTRINITY_API UGeoAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** INFO HOLDER **/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Info", meta = (DefaultToSelf = "WorldContextObject"))
	static UAbilityInfo* GetAbilityInfo(const UObject* WorldContextObject);
	static UAbilityInfo* GetAbilityInfo();
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Info", meta = (DefaultToSelf = "WorldContextObject"))
	static UStatusInfo* GetStatusInfo(const UObject* WorldContextObject);

	/** PARAMS STUFF **/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Effects")
	static FGameplayEffectContextHandle ApplyEffectFromEffectData(
		const TArray<TInstancedStruct<FEffectData>>& DataArray, UGeoAbilitySystemComponent* SourceASC,
		UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed);
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(const UEffectDataAsset* EffectDataAsset);
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(FGameplayTag AbilityTag);
	/** STATUS **/
	static bool ApplyStatusToTarget(UAbilitySystemComponent* pTargetASC, UAbilitySystemComponent* pSourceASC,
		const FGameplayTag& statusTag, int32 level);

	/** TAG **/
	static FGameplayTag GetGameplayTagFromRootTagString(const FString& StringOfTag);
	static FGameplayTag GetAbilityTagFromSpec(const FGameplayAbilitySpec& Spec);
	static FGameplayTag GetAbilityTagFromAbility(const UGameplayAbility& Ability);

	/** TEAM **/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetAllAgentsInTeam(const UObject* WorldContextObject, const FGenericTeamId& TeamId);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetAllAgentsWithRelationTowardsActor(const UObject* WorldContextObject, const AActor* Actor,
		ETeamAttitude::Type Attitude);

	/** TOOLBOX **/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Toolbox")
	static AActor* GetNearestActorFromList(const AActor* FromActor, const TArray<AActor*>& ActorList);

	/**
	 * Getters and setters for the context
	 */
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool IsBlockedHit(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool IsCriticalHit(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool IsSuccessfulDebuff(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetDebuffDamage(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetDebuffDuration(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetDebuffFrequency(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FVector GetDeathImpulseVector(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FVector GetKnockbackVector(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool GetIsRadialDamage(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetRadialDamageInnerRadius(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetRadialDamageOuterRadius(const FGameplayEffectContextHandle& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FVector GetRadialDamageOrigin(const FGameplayEffectContextHandle& effectContextHandle);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetIsBlockedHit(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const bool bIsBlockedHit);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetIsCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const bool bIsCriticalHit);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetStatusTag(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const FGameplayTag statusTag);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDebuffDamage(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const float debuffDamage);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDebuffFrequency(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const float debuffFrequency);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDebuffDuration(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const float debuffDuration);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDeathImpulseVector(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const FVector& inVector);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetKnockbackVector(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const FVector& inVector);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetIsRadialDamage(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const bool bIsRadialDamage);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageInnerRadius(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const float radialDamageInnerRadius);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageOuterRadius(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const float radialDamageOuterRadius);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageOrigin(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
		const FVector& inVector);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary")
	static UGeoAbilitySystemComponent* GetGeoAscFromActor(AActor* Actor);

	/**
	 * END context getter setters
	 */
};
