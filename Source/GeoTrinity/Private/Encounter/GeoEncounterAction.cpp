// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoEncounterAction.h"

#include "Encounter/GeoEncounter.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Interface/GeoHUDInterface.h"

void UGeoEncounterAction::Begin(FGeoEncounterPayload const& Payload)
{
	Execute(Payload);
	if (!bLatent)
	{
		Finish();
	}
}

void UGeoEncounterAction::Finish()
{
	AGeoEncounter* const Encounter = GetEncounter();
	if (ensureMsgf(Encounter, TEXT("Action %s finished outside an encounter"), *GetName()))
	{
		Encounter->OnActionFinished(*this);
	}
}

IGeoHUDInterface* UGeoEncounterAction::GetLocalHUD() const
{
	APlayerController const* const PlayerController = GetWorld()->GetFirstPlayerController();
	IGeoHUDInterface* const HUD = PlayerController ? Cast<IGeoHUDInterface>(PlayerController->GetHUD()) : nullptr;
	if (!HUD)
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no local player HUD"), *GetName());
	}

	return HUD;
}
