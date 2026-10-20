// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoSetTeleporterOpenAction.h"

#include "Actor/Buttons/GeoTeleporter.h"

void UGeoSetTeleporterOpenAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	if (ensureMsgf(Teleporter, TEXT("%s: no Teleporter set"), *GetName()))
	{
		Teleporter->SetOpen(bOpen);
	}
}
