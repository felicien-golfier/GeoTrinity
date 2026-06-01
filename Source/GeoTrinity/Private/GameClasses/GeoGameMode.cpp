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
	AGeoGameState* GameState = GetGameState<AGeoGameState>();
	if (!GameState || !GameState->IsMatchInProgress())
	{
		return;
	}
	APlayableCharacter* PlayableCharacter = Exiting ? Cast<APlayableCharacter>(Exiting->GetPawn()) : nullptr;
	if (!PlayableCharacter)
	{
		return;
	}
	GameState->NotifyPlayerDiedInFight();
}
