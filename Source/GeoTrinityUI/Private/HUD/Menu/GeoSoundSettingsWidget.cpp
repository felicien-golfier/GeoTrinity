// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoSoundSettingsWidget.h"

#include "AudioDevice.h"
#include "Components/Slider.h"
#include "Engine/World.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Settings/GeoGameUserSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UGeoGameUserSettings const* Settings = UGeoGameUserSettings::Get();
	GeneralVolumeSlider->SetValue(Settings->GetVolume(EGeoVolumeChannel::General));
	EffectsVolumeSlider->SetValue(Settings->GetVolume(EGeoVolumeChannel::Effects));
	MusicVolumeSlider->SetValue(Settings->GetVolume(EGeoVolumeChannel::Music));
	InterfaceVolumeSlider->SetValue(Settings->GetVolume(EGeoVolumeChannel::Interface));

	BackButton->OnClicked.AddUniqueDynamic(this, &UGeoSoundSettingsWidget::HandleBack);
	GeneralVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UGeoSoundSettingsWidget::HandleGeneralVolumeChanged);
	EffectsVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UGeoSoundSettingsWidget::HandleEffectsVolumeChanged);
	MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UGeoSoundSettingsWidget::HandleMusicVolumeChanged);
	InterfaceVolumeSlider->OnValueChanged.AddUniqueDynamic(this,
															&UGeoSoundSettingsWidget::HandleInterfaceVolumeChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoSoundSettingsWidget::GetInitialFocusWidget() const
{
	return BackButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoSoundSettingsWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::HandleGeneralVolumeChanged(float const Value)
{
	SetVolume(EGeoVolumeChannel::General, Value);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::HandleEffectsVolumeChanged(float const Value)
{
	SetVolume(EGeoVolumeChannel::Effects, Value);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::HandleMusicVolumeChanged(float const Value)
{
	SetVolume(EGeoVolumeChannel::Music, Value);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::HandleInterfaceVolumeChanged(float const Value)
{
	SetVolume(EGeoVolumeChannel::Interface, Value);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::SetVolume(EGeoVolumeChannel const Channel, float const Volume) const
{
	UGeoGameUserSettings* Settings = UGeoGameUserSettings::Get();
	Settings->SetVolume(Channel, Volume);
	FAudioDeviceHandle AudioDevice = GetWorld()->GetAudioDevice();
	if (AudioDevice)
	{
		Settings->ApplyVolumes(*AudioDevice);
	}
}
