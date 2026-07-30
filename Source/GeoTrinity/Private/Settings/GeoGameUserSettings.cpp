// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Settings/GeoGameUserSettings.h"

#include "Engine/Engine.h"

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
