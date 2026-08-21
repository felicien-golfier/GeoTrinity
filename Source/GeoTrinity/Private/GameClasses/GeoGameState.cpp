// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameState.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/World.h"
#include "GameClasses/GeoGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoGameState, Difficulty);
}

void AGeoGameState::SetDifficulty(EGeoDifficulty NewDifficulty)
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("SetDifficulty is server-only")) || Difficulty == NewDifficulty)
	{
		return;
	}
	if (IsMatchInProgress())
	{
		UE_LOG(LogTemp, Warning, TEXT("Difficulty change ignored — a fight is in progress"));
		return;
	}

	Difficulty = NewDifficulty;
	OnRep_Difficulty();
}

void AGeoGameState::OnRep_Difficulty()
{
	OnDifficultyChanged.Broadcast();
}

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

	// Out of a fight every death is independent: this player alone comes back, once their death has played out.
	if (!IsMatchInProgress())
	{
		FTimerHandle DeathTimer;
		GetWorld()->GetTimerManager().SetTimer(DeathTimer,
											   FTimerDelegate::CreateWeakLambda(&Player,
																				[this, &Player]
																				{
																					RespawnPlayer(Player);
																				}),
											   DeathTime, false);
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
	GeoLib::TeleportPlayersToTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_Entrance, CurrentArenaTag);
	RevivePlayers();
	RequestWaitingToStart();
}

void AGeoGameState::RespawnPlayer(APlayableCharacter& Player)
{
	// A fight starting and ending within DeathTime revives the whole group first, leaving nothing to bring back here.
	if (!Player.IsDead())
	{
		return;
	}

	TArray<AActor*> const SpawnPoints =
		GeoLib::GetTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_Entrance, CurrentArenaTag);
	if (!ensureMsgf(!SpawnPoints.IsEmpty(), TEXT("No TargetPoint.Entrance point tagged %s to respawn %s at"),
					*CurrentArenaTag.ToString(), *Player.GetName()))
	{
		return;
	}

	Player.SetActorLocation(SpawnPoints[FMath::RandHelper(SpawnPoints.Num())]->GetActorLocation());
	Player.Revive();
}

void AGeoGameState::RequestWaitingToStart() const
{
	AGeoGameMode* GeoGameMode = GetWorld()->GetAuthGameMode<AGeoGameMode>();
	if (ensureMsgf(GeoGameMode, TEXT("RequestWaitingToStart is server-only")))
	{
		GeoGameMode->RequestWaitingToStart();
	}
}
