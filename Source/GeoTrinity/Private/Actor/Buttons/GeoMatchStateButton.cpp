// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Buttons/GeoMatchStateButton.h"

#include "Actor/Arena/GeoArena.h"
#include "Engine/World.h"
#include "GameClasses/GeoGameMode.h"

void AGeoMatchStateButton::Press()
{
	AGeoGameMode* GeoGameMode = GetWorld()->GetAuthGameMode<AGeoGameMode>();
	if (!ensureMsgf(GeoGameMode, TEXT("AGeoMatchStateButton %s has no AGeoGameMode"), *GetName()))
	{
		return;
	}

	switch (StateRequest)
	{
	case EGeoMatchStateRequest::StartMatch:
		GeoGameMode->StartMatch();
		break;
	case EGeoMatchStateRequest::WaitingToStart:
		GeoGameMode->RequestWaitingToStart();
		break;
	case EGeoMatchStateRequest::WaitingPostMatch:
		GeoGameMode->RequestWaitingPostMatch();
		break;
	case EGeoMatchStateRequest::RespawnBosses:
		AGeoArena::RespawnAllBosses(this);
		break;
	}
}
