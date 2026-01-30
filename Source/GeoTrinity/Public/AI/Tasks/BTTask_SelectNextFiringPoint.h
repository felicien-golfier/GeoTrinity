// BTTask_SelectNextFiringPoint.h
#pragma once

#include "BTTask_SelectNextFiringPoint.generated.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CoreMinimal.h"

/**
 * Sets a Vector blackboard key to the next firing point location from AEnemyCharacter.FiringPoints (round-robin)
 */
UCLASS(DisplayName = "Select Next Firing Point")
class GEOTRINITY_API UBTTask_SelectNextFiringPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SelectNextFiringPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// Blackboard key to write the target location to
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetLocationKey;
};
