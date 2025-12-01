// GeoEnemyAIController.h
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "GeoEnemyAIController.generated.h"

class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardComponent;

UCLASS()
class GEOTRINITY_API AGeoEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    AGeoEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void OnPossess(APawn* InPawn) override;

    UBlackboardComponent* GetBlackboardComponent() const { return BlackboardComp; }
    UBehaviorTreeComponent* GetBehaviorComp() const { return BehaviorComp; }

protected:
    UPROPERTY(Transient)
    TObjectPtr<UBlackboardComponent> BlackboardComp;

    UPROPERTY(Transient)
    TObjectPtr<UBehaviorTreeComponent> BehaviorComp;
};
