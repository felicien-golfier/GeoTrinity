// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HudFunctionLibrary.generated.h"
#include "Kismet/BlueprintFunctionLibrary.h"

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
