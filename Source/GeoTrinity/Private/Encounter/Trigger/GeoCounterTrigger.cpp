// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoCounterTrigger.h"

#include "Encounter/GeoEncounter.h"

void UGeoCounterTrigger::OnArm_Implementation()
{
	GetEncounter()->OnCounterChanged.AddUObject(this, &UGeoCounterTrigger::OnCounterChanged);
}

void UGeoCounterTrigger::OnDisarm_Implementation()
{
	GetEncounter()->OnCounterChanged.RemoveAll(this);
}

void UGeoCounterTrigger::OnCounterChanged(FName const Counter, int32 const NewValue)
{
	if (Counter == CounterName)
	{
		FGeoEncounterPayload Payload;
		Payload.Count = NewValue;
		Fire(Payload);
	}
}
