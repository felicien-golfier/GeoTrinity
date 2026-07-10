// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoSoundSettingsWidget.h"

#include "Components/Slider.h"
#include "HUD/Menu/GeoMenuButton.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoSoundSettingsWidget: BackButton is not bound"));
		return;
	}
	if (!MasterVolumeSlider)
	{
		ensureMsgf(MasterVolumeSlider, TEXT("UGeoSoundSettingsWidget: MasterVolumeSlider is not bound"));
		return;
	}

	BackButton->OnClicked.AddDynamic(this, &UGeoSoundSettingsWidget::HandleBack);
	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UGeoSoundSettingsWidget::HandleMasterVolumeChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UGeoSoundSettingsWidget::HandleBack);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.RemoveDynamic(this, &UGeoSoundSettingsWidget::HandleMasterVolumeChanged);
	}
	Super::NativeDestruct();
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
void UGeoSoundSettingsWidget::HandleMasterVolumeChanged(float /*Value*/)
{
	// Placeholder: no sound-class/mixer convention exists in the project yet.
}
