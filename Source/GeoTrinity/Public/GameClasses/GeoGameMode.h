// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameStateBase.h"

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

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Notifies the game state when a playable character's controller disconnects during an active fight.
	 * Non-playable controller exits are silently ignored.
	 */
	virtual void Logout(AController* Exiting) override;

	/** Transitions the match back to WaitingToStart, resetting the round. */
	void RequestWaitingToStart() { SetMatchState(MatchState::WaitingToStart); }

	/** Transitions the match to WaitingPostMatch after the boss is defeated. */
	void RequestWaitingPostMatch() { SetMatchState(MatchState::WaitingPostMatch); }
};
