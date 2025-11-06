// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "GeoInputConfig.generated.h"

USTRUCT(BlueprintType)
struct FGeoInputAction
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	const class UInputAction* InputAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag {};
};

/**
 * Associates an input to a Gameplay tag (if we ever need to replicate it, it's probably simpler to replicate the tag
 * and an action type)
 */
UCLASS()
class GEOTRINITY_API UGeoInputConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(TitleProperty="{InputTag}"))
	TArray<FGeoInputAction> AbilityInputActions;

	UInputAction const* FindAbilityInputActionForTag(FGameplayTag const& inputTag, bool bLogNotFound) const;
	
};
