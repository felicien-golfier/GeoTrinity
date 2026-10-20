// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoTimerTrigger.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UGeoTimerTrigger::OnArm_Implementation()
{
	Firings = 0;
	GetWorld()->GetTimerManager().SetTimer(Timer, this, &UGeoTimerTrigger::OnTimer, Delay, bLooping);
}

void UGeoTimerTrigger::OnDisarm_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(Timer);
}

void UGeoTimerTrigger::OnTimer()
{
	FGeoEncounterPayload Payload;
	Payload.Count = ++Firings;
	Fire(Payload);
}
