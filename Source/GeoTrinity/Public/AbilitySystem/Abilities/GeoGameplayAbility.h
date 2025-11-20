// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GeoGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class GEOTRINITY_API UGeoGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	FGameplayTag GetAbilityTag() const;
	
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
	FGameplayTag StartupInputTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Status")
	FGameplayTag StatusToInflictTag {};
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Status", meta=(ClampMin="0", ClampMax="100"))
	uint8 ChanceToInflictStatus {0};
};
