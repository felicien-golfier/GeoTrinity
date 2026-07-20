// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameMode.h"

#include "Characters/GeoCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "GameClasses/GeoGameState.h"

AGeoGameMode::AGeoGameMode(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultPawnClass = AGeoCharacter::StaticClass();
}

void AGeoGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AGeoGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	AGeoGameState const* GeoGameState = GetGameState<AGeoGameState>();
	APlayableCharacter* PlayableCharacter = Exiting ? Cast<APlayableCharacter>(Exiting->GetPawn()) : nullptr;
	// Logout runs before the controller unpossesses, so the leaver would otherwise still read as standing.
	if (GeoGameState && GeoGameState->IsMatchInProgress() && PlayableCharacter)
	{
		PlayableCharacter->Death();
	}
}

bool AGeoGameMode::ReadyToStartMatch_Implementation()
{
	return false; // Only code can call StartMatch
}

void AGeoGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (!bStartPlayersAsSpectators && !MustSpectate(NewPlayer))
	{
		RestartPlayer(NewPlayer);
	}
}
