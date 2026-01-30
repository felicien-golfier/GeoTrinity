// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "HudFunctionLibrary.generated.h"

class UAbilitySystemComponent;
/**
 * A library of helper functions related to HUD
 */
UCLASS()
class GEOTRINITY_API UHudFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// DRAW //
	static bool ShouldDrawHUD(UObject const* WorldContextObject);

	// STATS //
	UFUNCTION(BlueprintCallable, Category = "Gas")
	static float GetHealthRatio(UAbilitySystemComponent const* AbilitySystemComponent);
};
