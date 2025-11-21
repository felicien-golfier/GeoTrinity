// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GeoAbilitySystemComponent.generated.h"

class UGeoGameplayAbility;
struct FGeoGameplayEffectContext;
/*
 * Ability system component tailored for the Geo game (2D chara top down)
 * */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	
	/** Create an EffectContext for the owner of this AbilitySystemComponent */
	FGeoGameplayEffectContext* MakeGeoEffectContext() const;
	
	/** Abilities **/
	void AddCharacterStartupAbilities(TArray<TSubclassOf<UGeoGameplayAbility>>& AbilitiesToGive);
	
	/** Input **/
	void AbilityInputTagPressed(FGameplayTag const& inputTag);
	void AbilityInputTagHeld(FGameplayTag const& inputTag);
	void AbilityInputTagReleased(FGameplayTag const& inputTag);
	
private:
	bool bStartupAbilitiesGiven {false};
};