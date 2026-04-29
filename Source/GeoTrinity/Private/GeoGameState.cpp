// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoGameState.h"

#include "Characters/EnemyCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (HasAuthority() && EnemiesToSpawn.Num() > 0)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;
		for (auto EnemyToSpawn : EnemiesToSpawn)
		{
			AEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacter>(
				EnemyToSpawn, FTransform(FVector(100, 100, ArbitraryCharacterZ)), SpawnParams);
			SpawnedEnemies.Add(SpawnedEnemy);
			OnEnemySpawned.Broadcast(SpawnedEnemy);
		}
	}
}
