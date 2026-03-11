// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"

#include "GeoGameState.generated.h"

class AEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AEnemyCharacter*, Enemy);

/**
 *
 */
UCLASS()
class GEOTRINITY_API AGeoGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void HandleMatchHasStarted() override;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemySpawned OnEnemySpawned;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	AEnemyCharacter* GetFirstEnemy() const { return SpawnedEnemies.Num() > 0 ? SpawnedEnemies[0] : nullptr; }

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Enemy")
	TArray<TObjectPtr<AEnemyCharacter>> SpawnedEnemies;

private:
	UPROPERTY(EditAnywhere, Category = "Enemy")
	TArray<TSubclassOf<AEnemyCharacter>> EnemiesToSpawn;
};
