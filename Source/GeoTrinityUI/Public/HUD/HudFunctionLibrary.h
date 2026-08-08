// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "HudFunctionLibrary.generated.h"

class UAbilitySystemComponent;
/**
 * A library of helper functions related to HUD
 */
UCLASS()
class GEOTRINITYUI_API UHudFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns false only on dedicated servers or without a world; true for standalone, client, or listen server. */
	static bool ShouldDrawHUD(UObject const* WorldContextObject);

	/** Returns Health / MaxHealth clamped to [0, 1]. Returns 0 if AbilitySystemComponent is null. */
	UFUNCTION(BlueprintCallable, Category = "Gas")
	static float GetHealthRatio(UAbilitySystemComponent const* AbilitySystemComponent);
};
