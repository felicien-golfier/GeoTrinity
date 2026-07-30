// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"

#include "GeoGameViewportClient.generated.h"

/**
 * Viewport client for GeoTrinity, owning couch-coop device assignment.
 *
 * `input.DeviceMappingPolicy=2` (Config/Windows/WindowsInput.ini) gives every device its own platform user, so each
 * gamepad is independently assignable. The engine routes input strictly by platform user and never falls back to
 * player 0, so this class decides which platform user each gamepad points at, from the one couch-coop setting in
 * UGeoGameUserSettings.
 *
 * Splitscreen rendering is force-disabled: every local player shares one view, framed by AGeoGameCamera.
 */
UCLASS()
class GEOTRINITY_API UGeoGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	/** Force-disables splitscreen so extra local players never split the view. */
	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance,
					  bool bCreateNewAudioDevice = true) override;

	/** Focuses every Slate user on the game viewport, so gamepads drive the game rather than menu navigation. */
	virtual void ReceivedFocus(FViewport* Viewport) override;

	/** Re-points a gamepad that drives nobody, then joins it as player 2 on Start; otherwise routes as usual. */
	virtual bool InputKey(FInputKeyEventArgs const& EventArgs) override;

	/**
	 * Makes the running game match UGeoGameUserSettings: with the setting on, the first gamepad gets a platform user
	 * of its own so it can own a second local player; every other gamepad — and all of them with the setting off —
	 * shares the keyboard's platform user, so gamepad and mouse both drive player 1. Turning the setting off also
	 * drops player 2, whose gamepad has just gone back to player 1.
	 */
	void ApplyCouchCoopSetting();

private:
	/**
	 * Adds a second local player owned by InputDevice, which must be the gamepad holding SecondPlayerUser.
	 *
	 * @return True when the player was created, meaning the press should be consumed.
	 */
	bool TryCreateSecondPlayer(FInputDeviceId InputDevice);

	/** Platform user of the gamepad driving player 2; allocated once and reused, so toggling cannot exhaust user ids. */
	FPlatformUserId SecondPlayerUser;
};
