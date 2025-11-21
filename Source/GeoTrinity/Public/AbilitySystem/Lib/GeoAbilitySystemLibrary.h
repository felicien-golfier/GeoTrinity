// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeoAbilitySystemLibrary.generated.h"

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
	/** DAMAGE PARAMS STUFF **/
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static FGameplayEffectContextHandle ApplyEffectFromDamageParams(FDamageEffectParams const& params);

	
	/** STATUS **/
	static bool ApplyStatusToTarget(UAbilitySystemComponent* pTargetASC, UAbilitySystemComponent* pSourceASC, FGameplayTag const& statusTag, int32 level);
	
	/** TAG **/
	static FGameplayTag GetGameplayTagFromRootTagString(FString const& StringOfTag);
	static FGameplayTag GetAbilityTagFromSpec(FGameplayAbilitySpec const& Spec);
	static FGameplayTag GetAbilityTagFromAbility(UGameplayAbility const& Ability);
	
	/**
	 * Getters and setters for the context
	 */
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static bool IsBlockedHit(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static bool IsCriticalHit(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static bool IsSuccessfulDebuff(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static float GetDebuffDamage(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static float GetDebuffDuration(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static float GetDebuffFrequency(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static FVector GetDeathImpulseVector(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static FVector GetKnockbackVector(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static bool GetIsRadialDamage(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static float GetRadialDamageInnerRadius(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static float GetRadialDamageOuterRadius(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category="MMAbilitySystemLibrary|GameplayEffects")
	static FVector GetRadialDamageOrigin(FGameplayEffectContextHandle const& effectContextHandle);
	
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetIsBlockedHit(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const bool bIsBlockedHit);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetIsCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const bool bIsCriticalHit);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetStatusTag(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const FGameplayTag statusTag);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetDebuffDamage(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const float debuffDamage);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetDebuffFrequency(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const float debuffFrequency);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetDebuffDuration(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const float debuffDuration);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetDeathImpulseVector(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, FVector const& inVector);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetKnockbackVector(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, FVector const& inVector);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetIsRadialDamage(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const bool bIsRadialDamage);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageInnerRadius(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const float radialDamageInnerRadius);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageOuterRadius(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, const float radialDamageOuterRadius);
	UFUNCTION(BlueprintCallable, Category="MMAbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageOrigin(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle, FVector const& inVector);
	/**
	 * END context getter setters
	 */
};
