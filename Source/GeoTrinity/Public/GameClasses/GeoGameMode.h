// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "GeoGameMode.generated.h"

class AEnemyCharacter;
class UStatusInfo;
/**
 * Game mode for GeoTrinity. Assigns starting player classes based on join order
 * and handles server-side round management.
 */
UCLASS()
class GEOTRINITY_API AGeoGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	/** Sets DefaultPawnClass to AGeoCharacter. */
	AGeoGameMode(FObjectInitializer const& ObjectInitializer);

	/** Drives deferred match-state work each frame. */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Downs a disconnecting player's character so the arena stops counting them among the living.
	 * Non-playable controller exits are silently ignored.
	 */
	virtual void Logout(AController* Exiting) override;
	/** Returns true when all connected players have been assigned a class and are ready to start. */
	virtual bool ReadyToStartMatch_Implementation() override;
	/** Assigns the joining player's class from join order and completes server-side pawn setup. */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** Transitions the match back to WaitingToStart, resetting the round. */
	void RequestWaitingToStart() { SetMatchState(MatchState::WaitingToStart); }

	/** Transitions the match to WaitingPostMatch after the boss is defeated. */
	void RequestWaitingPostMatch() { SetMatchState(MatchState::WaitingPostMatch); }
};
