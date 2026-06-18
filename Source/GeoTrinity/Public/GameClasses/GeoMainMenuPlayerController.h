// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "GeoMainMenuPlayerController.generated.h"

class UUserWidget;

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

	// The menu widget class lives in the UI module; held here as the engine base so gameplay never names the UI type.
	// Set the concrete UGeoMainMenuWidget subclass in Blueprint.
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> MenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> MenuWidget;
};
