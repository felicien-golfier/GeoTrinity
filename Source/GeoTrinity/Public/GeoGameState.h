// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"

#include "GeoGameState.generated.h"

class AEnemyCharacter;
/**
 *
 */
UCLASS()
class GEOTRINITY_API AGeoGameState : public AGameState
{
	GENERATED_BODY()

	virtual void HandleMatchHasStarted() override;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<AEnemyCharacter>> EnemiesToSpawn;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemyCharacter>> SpawnedEnemies;
};
