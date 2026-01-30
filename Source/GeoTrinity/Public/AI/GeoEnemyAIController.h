// GeoEnemyAIController.h
#pragma once

#include "AIController.h"
#include "CoreMinimal.h"

#include "GeoEnemyAIController.generated.h"

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

	UBlackboardComponent* GetBlackboardComponent() const { return BlackboardComp; }
	UBehaviorTreeComponent* GetBehaviorComp() const { return BehaviorComp; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<UBlackboardComponent> BlackboardComp;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTreeComponent> BehaviorComp;
};
