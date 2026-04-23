// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"

#include "GeoEnemyAIController.generated.h"


class UStateTree;
class UStateTreeAIComponent;
class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardComponent;

/**
 * AI controller for enemy characters. Starts a StateTree (via UStateTreeAIComponent) on possession
 * and leaves execution entirely to the tree tasks (FSTTask_FireProjectileAbility, etc.).
 */
UCLASS()
class GEOTRINITY_API AGeoEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:

	AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;
};
