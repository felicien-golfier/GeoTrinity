// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoGameState.h"

#include "Characters/EnemyCharacter.h"

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
			SpawnedEnemies.Add(
				GetWorld()->SpawnActor<AEnemyCharacter>(EnemyToSpawn, FTransform(FVector(0, 0, 50)), SpawnParams));
		}
	}
}