// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoMainMenuGameMode.h"

#include "GameClasses/GeoMainMenuPlayerController.h"

AGeoMainMenuGameMode::AGeoMainMenuGameMode(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AGeoMainMenuPlayerController::StaticClass();
}
