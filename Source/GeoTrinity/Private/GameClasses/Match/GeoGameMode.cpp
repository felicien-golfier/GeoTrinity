// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/Match/GeoGameMode.h"

#include "Characters/GeoCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "EngineUtils.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/PlayerStartPIE.h"

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

FString AGeoGameMode::InitNewPlayer(APlayerController* NewPlayerController, FUniqueNetIdRepl const& UniqueId,
									FString const& Options, FString const& Portal)
{
	bool bAnotherPlayerIn = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		bAnotherPlayerIn |= It->Get() != NewPlayerController;
	}

	return bSoloOnly && bAnotherPlayerIn ? TEXT("This map is solo only")
										 : Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

AActor* AGeoGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> UntaggedStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->IsA<APlayerStartPIE>())
		{
			return *It;
		}

		if (It->PlayerStartTag.IsNone())
		{
			UntaggedStarts.Add(*It);
		}
	}

	if (UntaggedStarts.IsEmpty())
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	return UntaggedStarts[FMath::RandRange(0, UntaggedStarts.Num() - 1)];
}

void AGeoGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (!bStartPlayersAsSpectators && !MustSpectate(NewPlayer))
	{
		RestartPlayer(NewPlayer);
	}
}
