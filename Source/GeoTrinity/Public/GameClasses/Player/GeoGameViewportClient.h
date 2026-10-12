// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"

#include "GeoGameViewportClient.generated.h"

/**
 * Viewport client for GeoTrinity, owning couch-coop device assignment.
 *
 * Keyboard and mouse always drive local player 0. `input.DeviceMappingPolicy=2` (Config/Windows/WindowsInput.ini)
 * gives every gamepad its own platform user, and the engine routes input strictly by platform user, so pointing a
 * gamepad at a character is just a matter of giving it that character's platform user.
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

	/** Settles gamepad ownership before the engine can drop the key, then joins an unassigned gamepad on Start. */
	virtual bool InputKey(FInputKeyEventArgs const& EventArgs) override;

	/**
	 * Seats every connected gamepad on a character: they start on the keyboard's, or one further along while
	 * UGeoGameUserSettings shifts them up. A gamepad already playing keeps its character, so a controller that comes
	 * back takes over the one its old device left behind; changing the setting instead re-seats the whole row in
	 * device order. A gamepad whose character does not exist yet creates it on Start, and a character left with no
	 * device at all is dropped.
	 */
	void ApplyCouchCoopSetting();

private:
	/** First character gamepads may take as of the last pass; a different one means the setting flipped. */
	int32 AppliedFirstCharacter = INDEX_NONE;
};
