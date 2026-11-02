// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoMenuGuideAction.h"

#include "Encounter/GeoEncounter.h"
#include "HUD/Interface/GeoHUDInterface.h"

void UGeoMenuGuideAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	OnCancel_Implementation();
	GetEncounter()->HoldUntilStepLeft(*this);
	if (IGeoHUDInterface* const HUD = GetLocalHUD(); HUD && Goal != EGeoMenuGuideGoal::None)
	{
		HUD->StartMenuGuide(Goal);
	}
}

void UGeoMenuGuideAction::OnCancel_Implementation()
{
	if (IGeoHUDInterface* const HUD = GetLocalHUD())
	{
		HUD->StopMenuGuide();
	}
}
