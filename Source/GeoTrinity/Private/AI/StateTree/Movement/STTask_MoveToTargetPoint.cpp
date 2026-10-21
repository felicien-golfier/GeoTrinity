// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_MoveToTargetPoint.h"

#include "AI/GeoEnemyAIController.h"
#include "Actor/Arena/GeoArena.h"
#include "Characters/GeoCharacter.h"
#include "StateTreeExecutionContext.h"
#include "Tool/UGeoGameplayLibrary.h"

EStateTreeRunStatus FSTTask_MoveToTargetPoint::EnterState(FStateTreeExecutionContext& Context,
														  FStateTreeTransitionResult const& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AGeoCharacter const* GeoCharacter =
		InstanceData.AIController ? Cast<AGeoCharacter>(InstanceData.AIController->GetPawn()) : nullptr;
	AGeoArena const* Arena = AGeoArena::GetArenaOfBoss(GeoCharacter);
	if (!ensureMsgf(Arena, TEXT("MoveToTargetPoint: %s was not spawned by an arena"), *GetNameSafe(GeoCharacter)))
	{
		return EStateTreeRunStatus::Failed;
	}

	TArray<AActor*> Points = GeoLib::GetTargetPoints(GeoCharacter, InstanceData.PointTag, Arena->ArenaTag);
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
