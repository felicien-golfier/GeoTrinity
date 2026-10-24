// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Tool/Team.h"

#include "GeoEnemySetup.generated.h"

class UStateTree;

/**
 * Variant of an enemy class, written where the enemy is spawned (AEnemyCharacter::ApplySetup) so a scripted room needs
 * one Blueprint for its look, not one per variant.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoEnemySetup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	ETeam Team = ETeam::Enemy;

	/** True: back at full health when it reaches zero (a training dummy). False: it dies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	bool bResetToFullLife = false;

	/** The tree it runs. None keeps the class's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	TObjectPtr<UStateTree> StateTree;

	/** The spell chain StateTree runs in its AI.Boss.Behaviour state (AEnemyCharacter::BehaviourStateTree), so a boss
	 * keeps the base tree's fight-start gate. None keeps the class's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	TObjectPtr<UStateTree> BehaviourStateTree;

	/** Abilities granted besides the class's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	TArray<FGameplayTag> AbilityTags;

	/** Max health. 0 keeps what the class's DefaultAttributes give. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	FScalableFloat MaxHealth;

	/** Share of max health it spawns with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup", meta = (ClampMin = "0.01", ClampMax = "1"))
	float StartHealthRatio = 1.f;

	/** Actor tag put on it, so a trigger can tell it from others of its class. None = no tag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	FName Tag;

	/** Distance in cm at which a player wakes it. Negative keeps its controller's AggroRadius. A hit wakes it from
	 * anywhere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEnemySetup")
	float AggroRadius = -1.f;
};
