// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_ChaseTarget.generated.h"

class AAIController;

/** Per-instance data for FSTTask_ChaseTarget. */
USTRUCT()
struct GEOTRINITY_API FSTTask_ChaseTargetInstanceData
{
	GENERATED_BODY()

	/** Bound to the running AGeoEnemyAIController (the schema's context is typed AAIController; cast at each use). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** 2D distance at which the pawn stops advancing while still facing the target. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float StopDistance = 300.f;
};

/**
 * Chases AIController::GetCurrentTarget() in a straight line — no navmesh, so destroyed arena tiles are ignored —
 * while facing it: sets AGeoCharacter::SetTargetYaw() each tick, which the character turns toward at its own
 * MaxRotationSpeed. Speed comes from the pawn's movement component. Never succeeds; transitions end it.
 */
USTRUCT(DisplayName = "Chase Target (Geo)", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_ChaseTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_ChaseTargetInstanceData;

	/** Declares FSTTask_ChaseTargetInstanceData as the per-execution instance data type so StateTree allocates the
	 * correct struct for each context. */
	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** Resolves the target, sets its facing as the pawn's target yaw, and moves toward it. Always returns Running
	 * while the pawn is valid. */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};
