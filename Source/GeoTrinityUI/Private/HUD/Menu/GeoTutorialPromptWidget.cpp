// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoTutorialPromptWidget.h"

#include "GameClasses/GeoGameInstance.h"
#include "HUD/Menu/Frame/GeoMenuButton.h"
#include "Settings/GeoGameUserSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTutorialPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	DoItButton->OnClicked.AddUniqueDynamic(this, &UGeoTutorialPromptWidget::HandleDoIt);
	SkipButton->OnClicked.AddUniqueDynamic(this, &UGeoTutorialPromptWidget::HandleSkip);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoTutorialPromptWidget::GetInitialFocusWidget() const
{
	return DoItButton;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTutorialPromptWidget::HandleDoIt()
{
	UGeoGameUserSettings::Get()->MarkTutorialPromptAnswered();
	UGeoGameInstance* const GameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (ensureMsgf(GameInstance, TEXT("%hs: GameInstance is not a UGeoGameInstance"), __FUNCTION__))
	{
		GameInstance->OpenTutorial();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTutorialPromptWidget::HandleSkip()
{
	UGeoGameUserSettings::Get()->MarkTutorialPromptAnswered();
	HandleBackAction();
}
