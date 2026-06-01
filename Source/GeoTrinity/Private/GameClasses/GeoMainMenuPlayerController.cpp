// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoMainMenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "HUD/Menu/GeoMainMenuWidget.h"

AGeoMainMenuPlayerController::AGeoMainMenuPlayerController(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer)
{
	SetShowMouseCursor(true);
}

void AGeoMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!ensureMsgf(MenuWidgetClass, TEXT("AGeoMainMenuPlayerController: MenuWidgetClass is not set")))
	{
		return;
	}

	MenuWidget = CreateWidget<UGeoMainMenuWidget>(this, MenuWidgetClass);
	if (!ensureMsgf(MenuWidget, TEXT("AGeoMainMenuPlayerController: Failed to create MenuWidget")))
	{
		return;
	}

	MenuWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}
