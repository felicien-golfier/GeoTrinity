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

	if (!ResumeButton)
	{
		ensureMsgf(ResumeButton, TEXT("UGeoPauseMenuWidget: ResumeButton is not bound"));
		return;
	}
	if (!AbilitiesButton)
	{
		ensureMsgf(AbilitiesButton, TEXT("UGeoPauseMenuWidget: AbilitiesButton is not bound"));
		return;
	}
	if (!SettingsButton)
	{
		ensureMsgf(SettingsButton, TEXT("UGeoPauseMenuWidget: SettingsButton is not bound"));
		return;
	}
	if (!ReturnToMainMenuButton)
	{
		ensureMsgf(ReturnToMainMenuButton, TEXT("UGeoPauseMenuWidget: ReturnToMainMenuButton is not bound"));
		return;
	}
	if (!QuitButton)
	{
		ensureMsgf(QuitButton, TEXT("UGeoPauseMenuWidget: QuitButton is not bound"));
		return;
	}
	if (!AbilitiesWidget)
	{
		ensureMsgf(AbilitiesWidget, TEXT("UGeoPauseMenuWidget: AbilitiesWidget is not bound"));
		return;
	}
	if (!SettingsWidget)
	{
		ensureMsgf(SettingsWidget, TEXT("UGeoPauseMenuWidget: SettingsWidget is not bound"));
		return;
	}

	ResumeButton->OnClicked.AddDynamic(this, &UGeoPauseMenuWidget::HandleResume);
	AbilitiesButton->OnClicked.AddDynamic(this, &UGeoPauseMenuWidget::HandleAbilities);
	SettingsButton->OnClicked.AddDynamic(this, &UGeoPauseMenuWidget::HandleSettings);
	ReturnToMainMenuButton->OnClicked.AddDynamic(this, &UGeoPauseMenuWidget::HandleReturnToMainMenu);
	QuitButton->OnClicked.AddDynamic(this, &UGeoPauseMenuWidget::HandleQuit);
	AbilitiesWidget->OnClosed.AddDynamic(this, &UGeoPauseMenuWidget::HandleSubPanelClosed);
	SettingsWidget->OnClosed.AddDynamic(this, &UGeoPauseMenuWidget::HandleSubPanelClosed);

	// Reset to the top-level buttons: the menu can close from anywhere (e.g. ESC while a sub-panel is open), and
	// this instance is reused on the next open.
	AbilitiesWidget->SetVisibility(ESlateVisibility::Collapsed);
	SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsVisible(true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::NativeDestruct()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleResume);
	}
	if (AbilitiesButton)
	{
		AbilitiesButton->OnClicked.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleAbilities);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleSettings);
	}
	if (ReturnToMainMenuButton)
	{
		ReturnToMainMenuButton->OnClicked.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleReturnToMainMenu);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleQuit);
	}
	if (AbilitiesWidget)
	{
		AbilitiesWidget->OnClosed.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleSubPanelClosed);
	}
	if (SettingsWidget)
	{
		SettingsWidget->OnClosed.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleSubPanelClosed);
	}
	Super::NativeDestruct();
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
