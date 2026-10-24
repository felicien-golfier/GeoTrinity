// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoStartEncounterAction.h"

#include "Encounter/GeoEncounter.h"

void UGeoStartEncounterAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	if (ensureMsgf(Target, TEXT("%s: no Target set"), *GetName()))
	{
		Target->StartAfterSkippingChain();
	}
}
