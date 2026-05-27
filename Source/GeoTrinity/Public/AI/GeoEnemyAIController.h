// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"

#include "GeoEnemyAIController.generated.h"


class UGeoAIBlackboardComponent;
class UStateTree;
class UStateTreeAIComponent;

/**
 * AI controller for enemy characters. Starts a StateTree (via UStateTreeAIComponent) on possession
 * and leaves execution entirely to the tree tasks (FSTTask_FireProjectileAbility, etc.).
 */
UCLASS()
class GEOTRINITY_API AGeoEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	/** Creates the UStateTreeAIComponent and UGeoAIBlackboardComponent subobjects. */
	AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UGeoAIBlackboardComponent> GeoBlackBoard;

	/** Starts the StateTree via StateTreeAIComponent after taking possession of InPawn. */
	virtual void OnPossess(APawn* InPawn) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;
};
