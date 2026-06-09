// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "GeoMainMenuPlayerController.generated.h"

class UGeoMainMenuWidget;

/**
 * Player controller for the main menu level. On BeginPlay it creates the main menu widget,
 * adds it to the viewport, and locks input to UI-only mode.
 */
UCLASS()
class GEOTRINITY_API AGeoMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGeoMainMenuPlayerController(FObjectInitializer const& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UGeoMainMenuWidget> MenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UGeoMainMenuWidget> MenuWidget;
};
