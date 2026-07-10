// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoSettingsWidget.h"

#include "HUD/Menu/GeoKeyBindingsWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoSoundSettingsWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SoundButton)
	{
		ensureMsgf(SoundButton, TEXT("UGeoSettingsWidget: SoundButton is not bound"));
		return;
	}
	if (!KeyBindingsButton)
	{
		ensureMsgf(KeyBindingsButton, TEXT("UGeoSettingsWidget: KeyBindingsButton is not bound"));
		return;
	}
	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoSettingsWidget: BackButton is not bound"));
		return;
	}
	if (!SoundWidget)
	{
		ensureMsgf(SoundWidget, TEXT("UGeoSettingsWidget: SoundWidget is not bound"));
		return;
	}
	if (!KeyBindingsWidget)
	{
		ensureMsgf(KeyBindingsWidget, TEXT("UGeoSettingsWidget: KeyBindingsWidget is not bound"));
		return;
	}

	SoundButton->OnClicked.AddDynamic(this, &UGeoSettingsWidget::HandleSound);
	KeyBindingsButton->OnClicked.AddDynamic(this, &UGeoSettingsWidget::HandleKeyBindings);
	BackButton->OnClicked.AddDynamic(this, &UGeoSettingsWidget::HandleBack);
	SoundWidget->OnClosed.AddDynamic(this, &UGeoSettingsWidget::HandleSubPanelClosed);
	KeyBindingsWidget->OnClosed.AddDynamic(this, &UGeoSettingsWidget::HandleSubPanelClosed);

	SoundWidget->SetVisibility(ESlateVisibility::Collapsed);
	KeyBindingsWidget->SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::NativeDestruct()
{
	if (SoundButton)
	{
		SoundButton->OnClicked.RemoveDynamic(this, &UGeoSettingsWidget::HandleSound);
	}
	if (KeyBindingsButton)
	{
		KeyBindingsButton->OnClicked.RemoveDynamic(this, &UGeoSettingsWidget::HandleKeyBindings);
	}
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UGeoSettingsWidget::HandleBack);
	}
	if (SoundWidget)
	{
		SoundWidget->OnClosed.RemoveDynamic(this, &UGeoSettingsWidget::HandleSubPanelClosed);
	}
	if (KeyBindingsWidget)
	{
		KeyBindingsWidget->OnClosed.RemoveDynamic(this, &UGeoSettingsWidget::HandleSubPanelClosed);
	}
	Super::NativeDestruct();
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
void UGeoSettingsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleSubPanelClosed()
{
	SoundWidget->SetVisibility(ESlateVisibility::Collapsed);
	KeyBindingsWidget->SetVisibility(ESlateVisibility::Collapsed);
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
	BackButton->SetVisibility(NewVisibility);
}
