// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoAddToCounterAction.h"

#include "Encounter/GeoEncounter.h"

void UGeoAddToCounterAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	GetEncounter()->AddToEncounterCounter(CounterName, Delta);
}
