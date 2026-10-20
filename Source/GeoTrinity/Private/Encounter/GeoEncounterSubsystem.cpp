// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoEncounterSubsystem.h"

void UGeoEncounterSubsystem::Raise(FGameplayTag const Tag, FGeoEncounterPayload Payload)
{
	Payload.Tag = Tag;
	OnRaised.Broadcast(Payload);
}
