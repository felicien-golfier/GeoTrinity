// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"

#include "GeoPlayerController.generated.h"

class UInputAction;
class UUserWidget;
struct FInputActionInstance;

/**
 * Player controller for GeoTrinity. Configures the camera view target and Enhanced Input
 * mapping context on BeginPlay. In non-shipping builds also drives the per-frame combat stats update.
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AGeoPlayerController(FObjectInitializer const& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

#if !UE_BUILD_SHIPPING
	virtual void Tick(float DeltaTime) override;
#endif

public:
	/** Returns the first local AGeoPlayerController found in World, or nullptr if none exists. */
	static AGeoPlayerController* GetLocalGeoPlayerController(UWorld const* World);

	/** Opens the pause menu if closed, or closes it (resumes) if already open. */
	void TogglePauseMenu();

	/** Closes the pause menu and restores game input mode. Called by the widget's Resume button. */
	void ClosePauseMenu();

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputAction> ToggleMenuAction;

	// Engine base so gameplay never names the UI type; concrete UGeoPauseMenuWidget set in Blueprint.
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidget;

	void HandleToggleMenu(FInputActionInstance const& Instance);
};
