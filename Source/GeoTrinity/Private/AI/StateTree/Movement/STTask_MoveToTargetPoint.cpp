// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_MoveToTargetPoint.h"

#include "AI/GeoEnemyAIController.h"
#include "Characters/GeoCharacter.h"
#include "StateTreeExecutionContext.h"
#include "Tool/UGeoGameplayLibrary.h"

EStateTreeRunStatus FSTTask_MoveToTargetPoint::EnterState(FStateTreeExecutionContext& Context,
														  FStateTreeTransitionResult const& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGeoCharacter const* GeoCharacter =
		InstanceData.AIController ? Cast<AGeoCharacter>(InstanceData.AIController->GetPawn()) : nullptr;
	if (!ensureMsgf(GeoCharacter, TEXT("MoveToTargetPoint: AIController has no GeoCharacter pawn"))
		|| !ensureMsgf(InstanceData.ArenaTag.IsValid(), TEXT("MoveToTargetPoint: %s's StateTree sets no ArenaTag"),
					   *GeoCharacter->GetName()))
	{
		return EStateTreeRunStatus::Failed;
	}

	TArray<AActor*> Points = GeoLib::GetTargetPoints(GeoCharacter, InstanceData.PointTag, InstanceData.ArenaTag);
	Points.RemoveAll(
		[&](AActor const* Point)
		{
			return FVector::Dist2D(Point->GetActorLocation(), GeoCharacter->GetActorLocation())
				<= InstanceData.AcceptanceRadius;
		});
	if (Points.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToTargetPoint: %s has no %s point to walk to"), *GetNameSafe(GeoCharacter),
			   *InstanceData.PointTag.ToString());
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.Destination = Points[FMath::RandHelper(Points.Num())]->GetActorLocation();
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_MoveToTargetPoint::Tick(FStateTreeExecutionContext& Context,
													float const /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGeoCharacter* GeoCharacter =
		InstanceData.AIController ? Cast<AGeoCharacter>(InstanceData.AIController->GetPawn()) : nullptr;
	if (!ensureMsgf(GeoCharacter, TEXT("MoveToTargetPoint: AIController has no GeoCharacter pawn")))
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector const ToDestination = InstanceData.Destination - GeoCharacter->GetActorLocation();
	if (ToDestination.Size2D() <= InstanceData.AcceptanceRadius)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	GeoCharacter->SetTargetYaw(ToDestination.ToOrientationRotator().Yaw);
	GeoCharacter->AddMovementInput(ToDestination.GetSafeNormal2D());
	return EStateTreeRunStatus::Running;
}
