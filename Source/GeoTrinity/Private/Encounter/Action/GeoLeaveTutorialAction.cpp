// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoLeaveTutorialAction.h"

#include "Engine/World.h"
#include "GameClasses/GeoGameInstance.h"

void UGeoLeaveTutorialAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	UGeoGameInstance* const GameInstance = GetWorld()->GetGameInstance<UGeoGameInstance>();
	if (ensureMsgf(GameInstance, TEXT("%s: the game instance is not a UGeoGameInstance"), *GetName()))
	{
		GameInstance->LeaveTutorial();
	}
}
