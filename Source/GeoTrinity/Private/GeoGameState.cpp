// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoGameState.h"

#include "Characters/EnemyCharacter.h"

void AGeoGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;
		SpawnedEnemy =
			GetWorld()->SpawnActor<AEnemyCharacter>(EnemyToSpawn, FTransform(FVector(0, 0, 50)), SpawnParams);
	}
}