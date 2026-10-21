// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_MoveToTargetPoint.generated.h"

class AAIController;

/** Per-instance data for FSTTask_MoveToTargetPoint. */
USTRUCT()
struct GEOTRINITY_API FSTTask_MoveToTargetPointInstanceData
{
	GENERATED_BODY()

	/** Bound to the running AGeoEnemyAIController (the schema's context is typed AAIController; cast at each use). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Purpose of the points walked between (TargetPoint.*), among those of the pawn's own arena. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	FGameplayTag PointTag;

	/** 2D distance at which the walk is over; a point the pawn already stands within is not picked. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "1.0"))
	float AcceptanceRadius = 100.f;

	/** The point picked on entering the state. */
	UPROPERTY(Transient)
	FVector Destination = FVector::ZeroVector;
};

/**
 * Walks the pawn in a straight line — no navmesh — to a random point of PointTag in its arena that it is not already
 * at, facing it: sets AGeoCharacter::SetTargetYaw() each tick, which the character turns toward at its own
 * MaxRotationSpeed. Speed comes from the pawn's movement component. Succeeds on arrival; Fails when no other point
 * exists.
 */
USTRUCT(DisplayName = "Move To Target Point (Geo)", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_MoveToTargetPoint : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_MoveToTargetPointInstanceData;

	/** Declares FSTTask_MoveToTargetPointInstanceData as the per-execution instance data type so StateTree allocates
	 * the correct struct for each context. */
	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** Picks the point to walk to. Failed when the pawn has no arena or no other point is left. */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
										   FStateTreeTransitionResult const& Transition) const override;
	/** Faces the point and moves toward it; Succeeded once within AcceptanceRadius. */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};
