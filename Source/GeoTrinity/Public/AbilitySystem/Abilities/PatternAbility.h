// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeoGameplayAbility.h"

#include "PatternAbility.generated.h"

/**
 *
 */
UCLASS()
class GEOTRINITY_API UPatternAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

public:
	TSubclassOf<UPattern> GetPatternClass() const { return PatternToLaunch; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Pattern")
	TSubclassOf<UPattern> PatternToLaunch;
};
