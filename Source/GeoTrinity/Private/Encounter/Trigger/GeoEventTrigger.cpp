// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoEventTrigger.h"

#include "Encounter/GeoEncounterSubsystem.h"

void UGeoEventTrigger::OnArm_Implementation()
{
	GetWorld()->GetSubsystem<UGeoEncounterSubsystem>()->OnRaised.AddUObject(this, &UGeoEventTrigger::OnRaised);
}

void UGeoEventTrigger::OnDisarm_Implementation()
{
	GetWorld()->GetSubsystem<UGeoEncounterSubsystem>()->OnRaised.RemoveAll(this);
}

void UGeoEventTrigger::OnRaised(FGeoEncounterPayload const& Payload)
{
	if (Payload.Tag.MatchesTag(Tag))
	{
		Fire(Payload);
	}
}
