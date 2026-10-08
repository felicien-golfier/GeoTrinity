// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/EnumRange.h"

#include "GeoGameUserSettings.generated.h"

class FAudioDevice;

/** One volume slider of the sound settings; UGameDataSettings::VolumeSoundClasses names the sound class each scales. */
UENUM(BlueprintType)
enum class EGeoVolumeChannel : uint8
{
	/** Scales every other channel on top of its own volume. */
	General,
	Effects,
	Music,
	Interface
};
ENUM_RANGE_BY_FIRST_AND_LAST(EGeoVolumeChannel, EGeoVolumeChannel::General, EGeoVolumeChannel::Interface);

DECLARE_MULTICAST_DELEGATE_OneParam(FGeoShowCombatStatsChangedSignature, bool /*bShow*/);

/**
 * Player-facing settings saved to GameUserSettings.ini: the couch-coop device choice (whether the first gamepad
 * drives a second local player, or shares player 1 with the keyboard and mouse, see UGeoGameViewportClient), the
 * volume of each EGeoVolumeChannel and what the HUD shows.
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

	/** Volume of Channel, 0 to 1. */
	float GetVolume(EGeoVolumeChannel Channel) const { return Volumes.FindChecked(Channel); }

	/** Sets the volume of Channel and persists it; call ApplyVolumes to hear it. */
	void SetVolume(EGeoVolumeChannel Channel, float Volume);

	/**
	 * Overrides each channel's sound class in the Default Base Sound Mix (Project Settings -> Audio) with its volume.
	 * An override lasts as long as its audio device: call it when the device is created and after every SetVolume.
	 */
	void ApplyVolumes(FAudioDevice& AudioDevice) const;

	/** Returns true when the HUD shows the combat stats panel. */
	bool ShowCombatStats() const { return bShowCombatStats; }

	/** Sets the value, persists it and broadcasts OnShowCombatStatsChanged. */
	void SetShowCombatStats(bool bShow);

	/** Fires after SetShowCombatStats, with the new value. */
	FGeoShowCombatStatsChangedSignature OnShowCombatStatsChanged;

private:
	UPROPERTY(Config)
	bool bUseFirstGamepadForSecondPlayer = false;

	/** General starts at half, so a first launch is quiet and the player turns it up or down from there. */
	UPROPERTY(Config)
	TMap<EGeoVolumeChannel, float> Volumes = {{EGeoVolumeChannel::General, .5f},
											  {EGeoVolumeChannel::Effects, 1.f},
											  {EGeoVolumeChannel::Music, 1.f},
											  {EGeoVolumeChannel::Interface, 1.f}};

	UPROPERTY(Config)
	bool bShowCombatStats = true;
};
