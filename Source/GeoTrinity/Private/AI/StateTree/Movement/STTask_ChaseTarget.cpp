// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_ChaseTarget.h"

#include "AI/GeoEnemyAIController.h"
#include "Characters/GeoCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTTask_ChaseTarget::Tick(FStateTreeExecutionContext& Context, float const /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGeoEnemyAIController* AIController = Cast<AGeoEnemyAIController>(InstanceData.AIController);
	AGeoCharacter* GeoCharacter = AIController ? Cast<AGeoCharacter>(AIController->GetPawn()) : nullptr;
	if (!ensureMsgf(GeoCharacter, TEXT("ChaseTarget: AIController has no GeoCharacter pawn")))
	{
		return EStateTreeRunStatus::Failed;
	}

	APlayableCharacter* Target = AIController->GetCurrentTarget();
	if (!Target)
	{
		return EStateTreeRunStatus::Running;
	}

	FVector const ToTarget = Target->GetActorLocation() - GeoCharacter->GetActorLocation();
	GeoCharacter->SetTargetYaw(ToTarget.ToOrientationRotator().Yaw);

	if (ToTarget.SizeSquared() > FMath::Square(InstanceData.StopDistance))
	{
		GeoCharacter->AddMovementInput(ToTarget.GetSafeNormal());
	}
	return EStateTreeRunStatus::Running;
}
