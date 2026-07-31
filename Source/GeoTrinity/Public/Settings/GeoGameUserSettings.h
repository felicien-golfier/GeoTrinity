// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"

#include "GeoGameUserSettings.generated.h"

/**
 * Player-facing settings saved to GameUserSettings.ini.
 * Currently holds only the couch-coop device choice: whether the first gamepad drives a second local player,
 * or shares player 1 with the keyboard and mouse (see UGeoGameViewportClient).
 */
UCLASS()
class GEOTRINITY_API UGeoGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	/** The engine's settings object, which DefaultEngine.ini points at this class. Never null in a running game. */
	static UGeoGameUserSettings* Get();

	/** Returns true when the first connected gamepad drives a second local player instead of sharing player 1 with the keyboard and mouse. */
	bool UseFirstGamepadForSecondPlayer() const { return bUseFirstGamepadForSecondPlayer; }

	/** Sets the value and persists it; call UGeoGameViewportClient::ApplyCouchCoopSetting to act on it. */
	void SetUseFirstGamepadForSecondPlayer(bool bEnabled);

private:
	UPROPERTY(Config)
	bool bUseFirstGamepadForSecondPlayer = false;
};
