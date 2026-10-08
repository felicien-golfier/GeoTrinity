// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Settings/GeoGameUserSettings.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Settings/GameDataSettings.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UGeoGameUserSettings* UGeoGameUserSettings::Get()
{
	UGeoGameUserSettings* Settings = GEngine ? Cast<UGeoGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
	checkf(Settings, TEXT("GameUserSettingsClassName must point at UGeoGameUserSettings in DefaultEngine.ini"));
	return Settings;
}

void UGeoGameUserSettings::SetUseFirstGamepadForSecondPlayer(bool bEnabled)
{
	bUseFirstGamepadForSecondPlayer = bEnabled;
	SaveSettings();
}

void UGeoGameUserSettings::SetVolume(EGeoVolumeChannel const Channel, float const Volume)
{
	Volumes.FindChecked(Channel) = Volume;
	SaveSettings();
}

void UGeoGameUserSettings::SetShowCombatStats(bool const bShow)
{
	bShowCombatStats = bShow;
	SaveSettings();
	OnShowCombatStatsChanged.Broadcast(bShow);
}

void UGeoGameUserSettings::ApplyVolumes(FAudioDevice& AudioDevice) const
{
	TSoftObjectPtr<USoundMix> const BaseSoundMix(GetDefault<UAudioSettings>()->DefaultBaseSoundMix);
	USoundMix* const VolumeMix = UGameDataSettings::GetLoadedDataAsset(BaseSoundMix);
	if (!ensureMsgf(VolumeMix, TEXT("Project Settings -> Audio -> Default Base Sound Mix is unset: volumes can't apply")))
	{
		return;
	}

	TMap<EGeoVolumeChannel, TSoftObjectPtr<USoundClass>> const& SoundClasses =
		GetDefault<UGameDataSettings>()->VolumeSoundClasses;
	for (EGeoVolumeChannel const Channel : TEnumRange<EGeoVolumeChannel>())
	{
		USoundClass* const SoundClass = UGameDataSettings::GetLoadedDataAsset(SoundClasses.FindRef(Channel));
		if (ensureMsgf(SoundClass, TEXT("Game Data Settings -> VolumeSoundClasses has no sound class for %s"),
					   *UEnum::GetValueAsString(Channel)))
		{
			AudioDevice.SetSoundMixClassOverride(VolumeMix, SoundClass, GetVolume(Channel), 1.f, 0.f, true);
		}
	}
}
