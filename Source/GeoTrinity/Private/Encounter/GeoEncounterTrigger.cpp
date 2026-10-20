// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoEncounterTrigger.h"

#include "Encounter/GeoEncounter.h"

void UGeoEncounterTrigger::Fire(FGeoEncounterPayload const& Payload)
{
	AGeoEncounter* const Encounter = GetEncounter();
	if (ensureMsgf(Encounter, TEXT("Trigger %s fired outside an encounter"), *GetName()))
	{
		Encounter->OnTriggerFired(*this, Payload);
	}
}
