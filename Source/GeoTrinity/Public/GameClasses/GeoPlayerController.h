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
 * mapping context once its local player is assigned. In non-shipping builds also drives the per-frame combat stats
 * update.
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	/** Shows the mouse cursor by default; all further setup (camera, input mapping) runs in ReceivedPlayer. */
	AGeoPlayerController(FObjectInitializer const& ObjectInitializer);

protected:
	/** Sets the camera view target, input mode, and activates the gameplay mapping context; seeds key bindings for the active keyboard layout on the first run. */
	virtual void ReceivedPlayer() override;
	/** Binds ToggleMenuAction to HandleToggleMenu via the Enhanced Input component. */
	virtual void SetupInputComponent() override;

#if !UE_BUILD_SHIPPING
	/** Shows the on-screen ping readout when Geo.ShowPing is set; ticks UGeoCombatStatsSubsystem when Geo.ShowCombatStats is enabled. */
	virtual void Tick(float DeltaTime) override;
#endif

public:
	/** Returns the first local AGeoPlayerController found in World, or nullptr if none exists. */
	static AGeoPlayerController* GetLocalGeoPlayerController(UWorld const* World);

	/**
	 * Shows this player's crosshair mouse cursor or hides it entirely, taking effect immediately.
	 *
	 * @param bVisible  True to draw the crosshair cursor, false to remove it (the gamepad aim cursor stands in for it).
	 */
	void SetMouseCursorVisible(bool bVisible);

	/** Opens the pause menu if closed, or closes it (resumes) if already open. */
	void TogglePauseMenu();

	/** Closes the pause menu and restores game input mode. Called by the widget's Resume button. */
	void ClosePauseMenu();

	/** Returns true while the pause menu is on screen (gameplay input suspended). */
	bool IsPauseMenuOpen() const;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	// Replaces InputMapping while the pause menu is open, so gameplay abilities cannot fire behind the menu.
	UPROPERTY(EditAnywhere, Category = "GeoInput")
	TSoftObjectPtr<UInputMappingContext> MenuInputMapping;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	TSoftObjectPtr<UInputAction> ToggleMenuAction;

	// Engine base so gameplay never names the UI type; concrete UGeoPauseMenuWidget set in Blueprint.
	UPROPERTY(EditAnywhere, Category = "GeoUI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidget;

	void HandleToggleMenu(FInputActionInstance const& Instance);
	void SetMenuInputMappingActive(bool bMenuActive);
	/** Puts input back in gameplay mode, keeping the non-obvious mouse-capture setting the game needs. */
	void SetGameplayInputMode();

	/**
	 * First-run keyboard layout detection: remaps every default key binding to the key sitting at the same
	 * physical position on the active keyboard layout (e.g. WASD becomes ZQSD on AZERTY) and saves the result.
	 * Skipped entirely once any binding has been customized (by this seeding or by the user).
	 */
	void SeedKeyBindingsForKeyboardLayout();
};
