// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoSetActorHiddenAction.h"

#include "GameFramework/Actor.h"

void UGeoSetActorHiddenAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	if (ensureMsgf(Actor, TEXT("%s: no Actor set"), *GetName()))
	{
		Actor->SetActorHiddenInGame(bHidden);
	}
}
