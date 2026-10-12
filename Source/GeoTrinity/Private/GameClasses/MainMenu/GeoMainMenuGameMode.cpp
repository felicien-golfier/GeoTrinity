// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/MainMenu/GeoMainMenuGameMode.h"

#include "GameClasses/MainMenu/GeoMainMenuPlayerController.h"

AGeoMainMenuGameMode::AGeoMainMenuGameMode(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AGeoMainMenuPlayerController::StaticClass();
}
