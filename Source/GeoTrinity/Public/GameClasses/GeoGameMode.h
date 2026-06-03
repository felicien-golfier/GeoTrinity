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
	AGeoGameMode(FObjectInitializer const& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void Logout(AController* Exiting) override;
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	void RequestWaitingToStart() { SetMatchState(MatchState::WaitingToStart); }
	void RequestWaitingPostMatch() { SetMatchState(MatchState::WaitingPostMatch); }
};
