// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/GeoEnemySetup.h"
#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayTagContainer.h"

#include "GeoSpawnEnemiesAction.generated.h"

class AEnemyCharacter;
class AGeoCharacter;

/**
 * Spawns Count enemies of EnemyClass, varied by Setup, on the target points tagged PurposeTag and ArenaTag (round
 * robin). Each one that dies comes back RespawnDelay seconds after its own death, where it died (0: never); the others
 * are untouched. Finishes at once and keeps working until its step is left (held by the encounter); bKillOnStepExit
 * then kills the ones still alive. Anything after it in the list runs right away. Run again while held (a repeating
 * rule): the previous run ends first, as a cancel does.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnEnemiesAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** The step was left, or the action runs again: stops every respawn; kills the living with bKillOnStepExit. */
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TSubclassOf<AEnemyCharacter> EnemyClass;

	/** Applied to every spawn before it starts (team, StateTree, abilities, health, tag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGeoEnemySetup Setup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Seconds between an enemy's death and its respawn on the spot; 0 spawns once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "0"))
	float RespawnDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "TargetPoint"))
	FGameplayTag PurposeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Arena"))
	FGameplayTag ArenaTag;

	/** True: leaving the step kills every one still alive (the tutorial's quest cut). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bKillOnStepExit = false;

private:
	void SpawnAt(FVector Location);

	/** Forgets the dead one and starts its own respawn timer at its location. */
	void OnCharacterDied(AGeoCharacter& Character);

	TArray<FTimerHandle> RespawnTimers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemyCharacter>> Spawned;
};
