// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_FaceTarget.h"

#include "AI/GeoEnemyAIController.h"
#include "Characters/GeoCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTTask_FaceTarget::Tick(FStateTreeExecutionContext& Context, float const /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGeoEnemyAIController* AIController = Cast<AGeoEnemyAIController>(InstanceData.AIController);
	AGeoCharacter* Pawn = AIController ? Cast<AGeoCharacter>(AIController->GetPawn()) : nullptr;
	if (!ensureMsgf(Pawn, TEXT("FaceTarget: AIController has no GeoCharacter pawn")))
	{
		return EStateTreeRunStatus::Failed;
	}

	APlayableCharacter* Target = AIController->GetCurrentTarget();
	if (!Target)
	{
		return EStateTreeRunStatus::Running;
	}

	FVector2D const ToTarget = FVector2D(Target->GetActorLocation()) - FVector2D(Pawn->GetActorLocation());
	Pawn->SetTargetYaw(FMath::Atan2(ToTarget.Y, ToTarget.X) * (180.f / PI));
	return EStateTreeRunStatus::Running;
}
