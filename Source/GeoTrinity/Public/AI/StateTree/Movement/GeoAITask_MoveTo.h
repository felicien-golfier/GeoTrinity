// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/AITask_MoveTo.h"

#include "GeoAITask_MoveTo.generated.h"

class UGeoAbilitySystemComponent;

/**
 * AITask_MoveTo variant that enables path recalculation on nav mesh invalidation.
 * The base class disables auto-repath, which prevents the enemy from replanning around dynamic obstacles
 * (e.g. pillars) that appear after the initial path is computed. This subclass re-enables it.
 *
 * Also adds MoveGameplayCueTag on the pawn's ASC for the duration of a move, with the path length as the cue's
 * RawMagnitude. The cue replicates, so every machine plays the move's cosmetics even though this task — like all
 * StateTree AI — only ever runs on the server.
 */
UCLASS()
class GEOTRINITY_API UGeoAITask_MoveTo : public UAITask_MoveTo
{
	GENERATED_BODY()

public:
	/** Opts into TickTask (bTickingTask = true) to enable per-frame facing updates that the base class does not provide. */
	UGeoAITask_MoveTo(FObjectInitializer const& ObjectInitializer);

	/** Looping cue added for the whole move and removed when it ends; its RawMagnitude carries the path length in
	 *  world units. Set by FSTTask_MoveTo. */
	FGameplayTag MoveGameplayCueTag;

protected:
	/** Enables path recalculation on nav mesh invalidation (disabled in the base class) and starts the move gameplay cue on the pawn's ASC. */
	virtual void PerformMove() override;
	/** Removes the active move gameplay cue from the pawn's ASC on task completion or cancellation. */
	virtual void OnDestroy(bool bInOwnerFinished) override;
	/** Faces the pawn toward its current movement direction via AGeoCharacter::SetTargetYaw(), which turns at the
	 *  character's own MaxRotationSpeed. Opted in via bTickingTask since the base class doesn't tick. */
	virtual void TickTask(float DeltaTime) override;

private:
	/** Adds MoveGameplayCueTag on the pawn's ASC, passing the path length as RawMagnitude. No-op when no cue is
	 *  configured, or when one is already added — PerformMove re-enters on every path replan, and AddGameplayCue
	 *  appends unconditionally, so an unguarded call would stack a second cue. */
	void AddMoveGameplayCue();

	/** ASC the move cue is currently added on; null when none is active. A move that never started (failed request,
	 *  already at goal) leaves it null, so OnDestroy removes nothing. */
	TWeakObjectPtr<UGeoAbilitySystemComponent> MoveCueASC;
};
