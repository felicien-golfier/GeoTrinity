// GeoEnemyAIController.h
#pragma once

#include "AIController.h"
#include "CoreMinimal.h"

#include "GeoEnemyAIController.generated.h"


class UStateTree;
class UStateTreeAIComponent;
class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardComponent;

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
