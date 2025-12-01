// BTTask_SelectNextFiringPoint.cpp

#include "AI/Tasks/BTTask_SelectNextFiringPoint.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Characters/EnemyCharacter.h"

UBTTask_SelectNextFiringPoint::UBTTask_SelectNextFiringPoint()
{
    NodeName = TEXT("Select Next Firing Point");
}

EBTNodeResult::Type UBTTask_SelectNextFiringPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (AAIController* AICon = OwnerComp.GetAIOwner())
    {
        if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AICon->GetPawn()))
        {
            FVector Location;
            if (Enemy->GetAndAdvanceNextFiringPointLocation(Location))
            {
                if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
                {
                    BB->SetValueAsVector(TargetLocationKey.SelectedKeyName, Location);
                    return EBTNodeResult::Succeeded;
                }
            }
        }
    }
    return EBTNodeResult::Failed;
}
