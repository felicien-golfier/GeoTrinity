// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "HudFunctionLibrary.generated.h"

class UAbilitySystemComponent;
/**
 * Blueprint-callable HUD utility functions shared across widgets that have no single natural owner.
 * Centralises the draw-gate check (hides the HUD on dedicated servers transparently) and common
 * attribute accessors so widgets can query health state without holding a direct ASC reference.
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
