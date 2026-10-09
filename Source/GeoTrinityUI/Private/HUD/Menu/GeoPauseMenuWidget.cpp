// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoPauseMenuWidget.h"

#include "GameClasses/GeoGameInstance.h"
#include "GameClasses/GeoPlayerController.h"
#include "HUD/Menu/GeoCharacterSheetWidget.h"
#include "HUD/Menu/GeoLeaderboardWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoSettingsWidget.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResumeButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::CloseMenu);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleSettings);
	LeaderboardButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleLeaderboard);
	ReturnToMainMenuButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleReturnToMainMenu);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleQuit);
	if (CharacterButton)
	{
		CharacterButton->OnClicked.AddUniqueDynamic(this, &UGeoPauseMenuWidget::HandleCharacter);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoPauseMenuWidget::GetInitialFocusWidget() const
{
	return ResumeButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoPauseMenuWidget::HandleBackAction()
{
	CloseMenu();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::CloseMenu()
{
	if (AGeoPlayerController* PlayerController = Cast<AGeoPlayerController>(GetOwningPlayer()))
	{
		PlayerController->ClosePauseMenu();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleCharacter()
{
	OpenPage(UGeoCharacterSheetWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleSettings()
{
	OpenPage(UGeoSettingsWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPauseMenuWidget::HandleLeaderboard()
{
	OpenPage(UGeoLeaderboardWidget::StaticClass());
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
