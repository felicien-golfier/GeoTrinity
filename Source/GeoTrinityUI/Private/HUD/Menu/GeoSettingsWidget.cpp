// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoSettingsWidget.h"

#include "HUD/Menu/GeoInterfaceSettingsWidget.h"
#include "HUD/Menu/GeoKeyBindingsWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoSoundSettingsWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SoundButton->OnClicked.AddUniqueDynamic(this, &UGeoSettingsWidget::HandleSound);
	KeyBindingsButton->OnClicked.AddUniqueDynamic(this, &UGeoSettingsWidget::HandleKeyBindings);
	InterfaceButton->OnClicked.AddUniqueDynamic(this, &UGeoSettingsWidget::HandleInterface);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoSettingsWidget::GetInitialFocusWidget() const
{
	return SoundButton;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleSound()
{
	OpenPage(UGeoSoundSettingsWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleKeyBindings()
{
	OpenPage(UGeoKeyBindingsWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleInterface()
{
	OpenPage(UGeoInterfaceSettingsWidget::StaticClass());
}
