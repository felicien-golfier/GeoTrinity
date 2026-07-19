// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_ChaseTarget.generated.h"

class AAIController;

/** Per-instance data for FSTTask_ChaseTarget. */
USTRUCT()
struct GEOTRINITY_API FSTTask_ChaseTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Player class chased in priority; falls back to the nearest live player when none of this class is alive. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	EPlayerClass PreferredTargetClass = EPlayerClass::Square;

	/** 2D distance at which the pawn stops advancing while still facing the target. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float StopDistance = 150.f;
};

/**
 * Chases the preferred player class in a straight line — no navmesh, so destroyed arena tiles are ignored — while
 * keeping the pawn facing its target (control rotation; pawns use bUseControllerRotationYaw). Speed comes from the
 * pawn's movement component. Never succeeds; transitions end it.
 */
USTRUCT(DisplayName = "Chase Target (Geo)", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_ChaseTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_ChaseTargetInstanceData;

	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** Resolves the target and rotates/moves the pawn toward it. Always returns Running while the pawn is valid. */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};
