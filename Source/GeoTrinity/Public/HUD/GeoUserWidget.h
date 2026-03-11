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
	void InitFromHUD(AGeoHUD* GeoHUD);

	UFUNCTION(BlueprintImplementableEvent)
	void BindCallbacksFromHUD(AGeoHUD* GeoHUD);
};
