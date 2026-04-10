// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoUserWidget.generated.h"

class AGeoHUD;
/**
 * A derived class to allow easy binding of stuff
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
