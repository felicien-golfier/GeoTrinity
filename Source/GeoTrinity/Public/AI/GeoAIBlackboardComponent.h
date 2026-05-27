// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoAIBlackboardComponent.generated.h"

/**
 * All persistent AI state for the enemy StateTree.
 * Exposed as a struct so FSTTask_UpdateBlackboard can write any subset of fields from the editor.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoAIBlackboardData
{
	GENERATED_BODY()

	/** Last selected firing point index — written by FSTTask_SelectNextFiringPoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LastFiringPointIndex = 0;

	/** Cycle counter — written by FSTTask_SendEventAfterNCycles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CycleCount = 0;
};

/**
 * Persistent AI state for the enemy StateTree. Attached to AGeoEnemyAIController.
 * The StateTree schema resolves UActorComponent subclasses automatically, so any task
 * can link to this via TStateTreeExternalDataHandle<UGeoAIBlackboardComponent>.
 */
UCLASS()
class GEOTRINITY_API UGeoAIBlackboardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	FGeoAIBlackboardData Data;
};
