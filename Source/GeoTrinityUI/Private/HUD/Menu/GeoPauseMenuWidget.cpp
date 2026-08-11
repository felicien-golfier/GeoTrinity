// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoPauseMenuWidget.h"

#include "GameClasses/GeoGameInstance.h"
#include "GameClasses/GeoPlayerController.h"
#include "HUD/Menu/GeoAbilityDescriptionsWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoSettingsWidget.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResumeButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleResume);
	AbilitiesButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleAbilities);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleSettings);
	ReturnToMainMenuButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleReturnToMainMenu);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleQuit);
	AbilitiesWidget->OnClosed.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleSubPanelClosed);
	SettingsWidget->OnClosed.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleSubPanelClosed);

	// Reset to the top-level buttons: the menu can close from anywhere (e.g. ESC while a sub-panel is open), and
	// this instance is reused on the next open.
	AbilitiesWidget->SetVisibility(ESlateVisibility::Collapsed);
	SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsVisible(true);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoPauseMenuWidget::GetInitialFocusWidget() const
{
	return ResumeButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoPauseMenuWidget::HandleBackAction()
{
	HandleResume();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleResume()
{
	if (AGeoPlayerController* PlayerController = Cast<AGeoPlayerController>(GetOwningPlayer()))
	{
		PlayerController->ClosePauseMenu();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleAbilities()
{
	OpenSubPanel(AbilitiesWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleSettings()
{
	OpenSubPanel(SettingsWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleReturnToMainMenu()
{
	if (UGeoGameInstance* GameInstance = Cast<UGeoGameInstance>(GetGameInstance()))
	{
		GameInstance->LeaveSessionAndReturnToMenu();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleQuit()
{
#if WITH_EDITOR
	// QuitGame's "quit" console command has no real process to exit in PIE; stop the PIE session directly instead.
	if (GEditor && GEditor->IsPlayingSessionInEditor())
	{
		GEditor->RequestEndPlayMap();
		return;
	}
#endif
	UGeoGameInstance* GameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (!ensureMsgf(GameInstance, TEXT("UGeoPauseMenuWidget::HandleQuit: GameInstance is not a UGeoGameInstance")))
	{
		return;
	}
	GameInstance->QuitGame();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleSubPanelClosed()
{
	AbilitiesWidget->SetVisibility(ESlateVisibility::Collapsed);
	SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsVisible(true);
	ResumeButton->SetFocus();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::OpenSubPanel(UGeoMenuPanelWidget* SubPanel)
{
	SetButtonsVisible(false);
	SubPanel->SetVisibility(ESlateVisibility::Visible);
	SubPanel->SetFocus();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::SetButtonsVisible(bool bVisible)
{
	ESlateVisibility const NewVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	ResumeButton->SetVisibility(NewVisibility);
	AbilitiesButton->SetVisibility(NewVisibility);
	SettingsButton->SetVisibility(NewVisibility);
	ReturnToMainMenuButton->SetVisibility(NewVisibility);
	QuitButton->SetVisibility(NewVisibility);
}
