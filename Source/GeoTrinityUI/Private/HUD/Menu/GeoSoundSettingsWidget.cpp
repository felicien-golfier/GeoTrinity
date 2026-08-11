// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoSoundSettingsWidget.h"

#include "Components/Slider.h"
#include "HUD/Menu/GeoMenuButton.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BackButton->OnClicked.AddUniqueDynamic(this, &UGeoSoundSettingsWidget::HandleBack);
	MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UGeoSoundSettingsWidget::HandleMasterVolumeChanged);
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
