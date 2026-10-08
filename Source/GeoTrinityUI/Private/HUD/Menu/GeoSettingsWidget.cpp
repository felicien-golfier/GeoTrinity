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
	BackButton->OnClicked.AddUniqueDynamic(this, &UGeoSettingsWidget::HandleBack);
	SoundWidget->OnClosed.AddUniqueDynamic(this, &UGeoSettingsWidget::HandleSubPanelClosed);
	KeyBindingsWidget->OnClosed.AddUniqueDynamic(this, &UGeoSettingsWidget::HandleSubPanelClosed);
	InterfaceWidget->OnClosed.AddUniqueDynamic(this, &UGeoSettingsWidget::HandleSubPanelClosed);

	// Reset to the chooser: the menu can close from anywhere (e.g. ESC while a sub-panel is open), and this
	// instance is reused on the next open.
	SoundWidget->SetVisibility(ESlateVisibility::Collapsed);
	KeyBindingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	InterfaceWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsVisible(true);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoSettingsWidget::GetInitialFocusWidget() const
{
	return SoundButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoSettingsWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleSound()
{
	OpenSubPanel(SoundWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleKeyBindings()
{
	OpenSubPanel(KeyBindingsWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleInterface()
{
	OpenSubPanel(InterfaceWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleSubPanelClosed()
{
	SoundWidget->SetVisibility(ESlateVisibility::Collapsed);
	KeyBindingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	InterfaceWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsVisible(true);
	SoundButton->SetFocus();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::OpenSubPanel(UGeoMenuPanelWidget* SubPanel)
{
	SetButtonsVisible(false);
	SubPanel->SetVisibility(ESlateVisibility::Visible);
	SubPanel->SetFocus();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::SetButtonsVisible(bool bVisible)
{
	ESlateVisibility const NewVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	SoundButton->SetVisibility(NewVisibility);
	KeyBindingsButton->SetVisibility(NewVisibility);
	InterfaceButton->SetVisibility(NewVisibility);
	BackButton->SetVisibility(NewVisibility);
}
