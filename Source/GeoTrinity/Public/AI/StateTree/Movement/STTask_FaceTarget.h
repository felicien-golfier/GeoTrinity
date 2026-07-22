// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_FaceTarget.generated.h"

class AAIController;

/** Per-instance data for FSTTask_FaceTarget. */
USTRUCT()
struct GEOTRINITY_API FSTTask_FaceTargetInstanceData
{
	GENERATED_BODY()

	/** Bound to the running AGeoEnemyAIController (the schema's context is typed AAIController; cast at each use). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;
};

/**
 * Turns the pawn to face AIController::GetCurrentTarget() without moving it — sets AGeoCharacter::SetTargetYaw()
 * each tick, which the character turns toward at its own MaxRotationSpeed. Never succeeds; transitions end it.
 */
USTRUCT(DisplayName = "Face Target (Geo)", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_FaceTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_FaceTargetInstanceData;

	/** Declares FSTTask_FaceTargetInstanceData as the per-execution instance data type so StateTree allocates the
	 * correct struct for each context. */
	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** Resolves the target and sets its facing as the pawn's target yaw. Always returns Running while the pawn is
	 * valid. */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};
