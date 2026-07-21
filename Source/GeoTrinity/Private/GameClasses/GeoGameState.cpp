// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameState.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/World.h"
#include "GameClasses/GeoGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (!GeoLib::IsServer(this))
	{
		return;
	}

	FightPlayers.Reset();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayableCharacter* Player = It->IsValid() ? Cast<APlayableCharacter>((*It)->GetPawn()) : nullptr;
		if (IsValid(Player) && !Player->IsDead())
		{
			FightPlayers.Add(Player);
		}
	}
}

void AGeoGameState::OnRep_MatchState()
{
	if (PreviousMatchState != MatchState && PreviousMatchState == MatchState::InProgress && GeoLib::IsServer(this))
	{
		GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
		RevivePlayers();
	}
	OnMatchStateChanged.Broadcast(MatchState, PreviousMatchState);
	Super::OnRep_MatchState();
}

void AGeoGameState::RevivePlayers() const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayableCharacter* Player = It->IsValid() ? Cast<APlayableCharacter>((*It)->GetPawn()) : nullptr)
		{
			Player->Revive();
		}
	}
}

void AGeoGameState::NotifyPlayerDied(APlayableCharacter& Player)
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("NotifyPlayerDied is server-only")))
	{
		return;
	}

	// Out of a fight every death is independent: stand the player straight back up where they fell.
	if (!IsMatchInProgress())
	{
		Player.Revive();
		return;
	}

	// In a fight the player stays down; the group only comes back once everyone who started the fight is down.
	if (AreFightPlayersDead())
	{
		OnWipe.Broadcast(DeathTime);
		GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AGeoGameState::RespawnGroup, DeathTime, false);
	}
}

bool AGeoGameState::AreFightPlayersDead() const
{
	for (TWeakObjectPtr<APlayableCharacter> const& Player : FightPlayers)
	{
		if (Player.IsValid() && !Player->IsDead())
		{
			return false;
		}
	}
	return true;
}

void AGeoGameState::RespawnGroup()
{
	GeoLib::TeleportPlayersToTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_Entrance, CheckpointTag);
	RevivePlayers();
	RequestWaitingToStart();
}

void AGeoGameState::RequestWaitingToStart() const
{
	AGeoGameMode* GeoGameMode = GetWorld()->GetAuthGameMode<AGeoGameMode>();
	if (ensureMsgf(GeoGameMode, TEXT("RequestWaitingToStart is server-only")))
	{
		GeoGameMode->RequestWaitingToStart();
	}
}
