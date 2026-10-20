// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoOpenGemsPageAction.h"

#include "HUD/Interface/GeoHUDInterface.h"

void UGeoOpenGemsPageAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	if (IGeoHUDInterface* const HUD = GetLocalHUD())
	{
		HUD->OpenGemsPage(bForge);
	}
}
