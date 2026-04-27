// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoUserWidget.generated.h"

class AGeoHUD;
/**
 * Base widget class for GeoTrinity HUD screens. Provides the InitFromHUD / BindCallbacksFromHUD
 * contract so Blueprint subclasses can connect to AGeoHUD delegates without manually looking up the HUD.
 * All player-facing overlay widgets should inherit from this class.
 */
UCLASS()
class GEOTRINITY_API UGeoUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the HUD reference and fires BindCallbacksFromHUD so BP subclasses can connect to HUD delegates. */
	void InitFromHUD(AGeoHUD* GeoHUD);

	/** Blueprint event called after InitFromHUD. Implement in BP to bind to delegates exposed by AGeoHUD. */
	UFUNCTION(BlueprintImplementableEvent)
	void BindCallbacksFromHUD(AGeoHUD* GeoHUD);
};
