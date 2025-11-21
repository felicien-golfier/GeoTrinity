// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Abilities/GeoGameplayAbility.h"

#include "GeoDamageGameplayAbility.generated.h"

/**
 * Base class for a damaging ability
 */
UCLASS()
class GEOTRINITY_API UGeoDamageGameplayAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void CauseDamage(AActor* targetActor) const;


	UFUNCTION(BlueprintPure, BlueprintNativeEvent)
	float GetDamageAtLevel(int32 Level) const;
	UFUNCTION(BlueprintPure)
	float GetDamage() const;
	
protected:
	float GetCooldown(int32 level = 1) const;
	UFUNCTION(BlueprintPure)
	FDamageEffectParams MakeDamageEffectParamsFromClassDefaults(AActor const* pTargetActor) const;
	UFUNCTION(BlueprintPure)
	FVector GetDirectionFromCauseToTarget(FDamageEffectParams const& damageEffectParams, const float pitchOverride = 0.f) const;
	
	/** Returns a zero vector if dice roll decided there was no knockback. Takes into account the knockback magnitude **/
	UFUNCTION(BlueprintPure)
	FVector ComputeKnockbackVector(FVector const& originPoint, FVector const& destinationPoint, bool bPrintLog = false) const;
	UFUNCTION(BlueprintPure)
	FVector ComputeDeathImpulseVector(FVector const& originPoint, FVector const& destinationPoint) const;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	FScalableFloat DamageAmount;

	/** Death **/
	UPROPERTY(EditDefaultsOnly, Category="Damage")
	float DeathImpulseMagnitude = 2500.f;

	/** Knockback **/
	UPROPERTY(EditDefaultsOnly, Category="Damage", meta=(ClampMin="0", ClampMax="100"))
	uint8 KnockbackChancePercent = 0;
	UPROPERTY(EditDefaultsOnly, Category="Damage")
	float KnockbackMagnitude = 2500.f;
	
	/** Radial Damage **/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radial Damage")
	bool bIsRadialDamage {false};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radial Damage", meta = (EditCondition = "bIsRadialDamage"))
	float RadialDamageInnerRadius {0.f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Radial Damage", meta = (EditCondition = "bIsRadialDamage"))
	float RadialDamageOuterRadius {0.f};
};
