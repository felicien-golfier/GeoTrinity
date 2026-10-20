// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoActorDiedTrigger.h"

#include "Characters/GeoCharacter.h"
#include "EngineUtils.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GeoTrinity/GeoTrinity.h"

void UGeoActorDiedTrigger::OnArm_Implementation()
{
	Deaths = 0;
	ensureMsgf(Watched.Class, TEXT("%s: nothing watched"), *GetName());
	AGeoGameState* const GameState = GetWorld()->GetGameState<AGeoGameState>();
	if (GameState)
	{
		GameState->OnCharacterDied.AddUObject(this, &UGeoActorDiedTrigger::OnCharacterDied);
	}
	else
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no game state to watch deaths on"), *GetName());
	}
}

void UGeoActorDiedTrigger::OnDisarm_Implementation()
{
	AGeoGameState* const GameState = GetWorld()->GetGameState<AGeoGameState>();
	if (GameState)
	{
		GameState->OnCharacterDied.RemoveAll(this);
	}
}

void UGeoActorDiedTrigger::OnCharacterDied(AGeoCharacter& Character)
{
	if (Watched.Matches(Character))
	{
		int32 Alive = 0;
		for (TActorIterator<AGeoCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->IsDead() && Watched.Matches(**It))
			{
				++Alive;
			}
		}

		FGeoEncounterPayload Payload;
		Payload.Actor = &Character;
		Payload.Count = ++Deaths;
		Payload.Value = Alive;
		Fire(Payload);
	}
}
