// GeoEnemyAIController.cpp

#include "AI/GeoEnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/EnemyCharacter.h"

AGeoEnemyAIController::AGeoEnemyAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
}

void AGeoEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (const AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(InPawn))
    {
        if (EnemyChar->BehaviorTree)
        {
            if (EnemyChar->BehaviorTree->BlackboardAsset)
            {
                BlackboardComp->InitializeBlackboard(*EnemyChar->BehaviorTree->BlackboardAsset);
            }
            BehaviorComp->StartTree(*EnemyChar->BehaviorTree);
        }
    }
}
