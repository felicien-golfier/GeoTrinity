// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoShowAbilityPageAction.h"

#include "HUD/Interface/GeoHUDInterface.h"

void UGeoShowAbilityPageAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	if (IGeoHUDInterface* const HUD = GetLocalHUD())
	{
		HUD->ShowAbilityPage(AbilityTag);
	}
}
