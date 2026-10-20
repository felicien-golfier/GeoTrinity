// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoSetMovementLockedAction.h"

#include "Encounter/GeoEncounterSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UGeoSetMovementLockedAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	APlayerController* const PlayerController = GetWorld()->GetFirstPlayerController();
	UGeoEncounterSubsystem* const Subsystem = GetWorld()->GetSubsystem<UGeoEncounterSubsystem>();
	if (ensureMsgf(PlayerController, TEXT("%s: no local player controller"), *GetName())
		&& Subsystem->bMovementLocked != bLocked)
	{
		Subsystem->bMovementLocked = bLocked;
		PlayerController->SetIgnoreMoveInput(bLocked);
	}
}

void UGeoSetMovementLockedAction::OnSkipped_Implementation()
{
	Execute_Implementation(FGeoEncounterPayload());
}
