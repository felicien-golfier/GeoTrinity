// STTask_SelectNextFiringPoint.cpp

#include "AI/StateTree/STTask_SelectNextFiringPoint.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "Characters/EnemyCharacter.h"

EStateTreeRunStatus FSTTask_SelectNextFiringPoint::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AActor const* Actor = Cast<AActor>(Context.GetOwner());
	if (!Actor)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (AController const* Controller = Cast<AController>(Actor))
	{
		Actor = Controller->GetPawn();
	}

	AEnemyCharacter* Enemy = const_cast<AEnemyCharacter*>(Cast<AEnemyCharacter>(Actor));
	if (!Enemy)
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector Location;
	if (Enemy->GetAndAdvanceNextFiringPointLocation(Location))
	{
		InstanceData.TargetLocation = Location;
		return EStateTreeRunStatus::Unset;
	}

	return EStateTreeRunStatus::Failed;
}
