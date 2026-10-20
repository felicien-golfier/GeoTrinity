// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoDelayAction.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UGeoDelayAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	GetWorld()->GetTimerManager().SetTimer(Timer, this, &UGeoDelayAction::Finish,
										   FMath::Max(Duration, UE_KINDA_SMALL_NUMBER));
}

void UGeoDelayAction::OnCancel_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(Timer);
}
