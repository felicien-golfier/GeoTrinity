// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/AITask_MoveTo.h"

#include "GeoAITask_MoveTo.generated.h"

class UAudioComponent;
class UCurveFloat;
class USoundBase;

/**
 * AITask_MoveTo variant that enables path recalculation on nav mesh invalidation.
 * The base class disables auto-repath, which prevents the enemy from replanning around dynamic obstacles
 * (e.g. pillars) that appear after the initial path is computed. This subclass re-enables it.
 *
 * Also plays a looping move sound whose pitch follows PitchCurve across the move's travel time, so the sound
 * speeds up / slows down with the distance travelled without changing the clip, and a one-shot EndSound when the
 * move ends.
 */
UCLASS()
class GEOTRINITY_API UGeoAITask_MoveTo : public UAITask_MoveTo
{
	GENERATED_BODY()

public:
	UGeoAITask_MoveTo(const FObjectInitializer& ObjectInitializer);

	/** Looping sound played for the duration of a move. Its pitch is driven by PitchCurve over the move. Set by
	 * FSTTask_MoveTo. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> MoveSound;

	/** Pitch multiplier for MoveSound sampled at the 0..1 move progress (X=0 start, X=1 arrival). Set by
	 * FSTTask_MoveTo. */
	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> PitchCurve;

	/** One-shot sound played at the pawn when the move ends. Set by FSTTask_MoveTo. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> EndSound;

protected:
	virtual void PerformMove() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	/** Starts MoveSound as a loop on the pawn at the move's start. No-op if a loop is already playing (path replans
	 *  re-enter here) or on a dedicated server. */
	void PlayMoveSound();

	/** Plays EndSound once at the pawn's location. No-op when no end sound is set or on a dedicated server. */
	void PlayEndSound();

	/** Pitch multiplier for the given 0..1 move progress: PitchCurve sampled at Alpha, or 1 when no curve is set. */
	float GetMoveSoundPitch(float Alpha) const;

	/** Active move-sound loop, kept alive across path replans; null when no move sound is playing. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MoveSoundComponent;

	/** Time spent moving since the loop started, used to compute 0..1 progress against MoveSoundTravelTime. */
	float MoveSoundElapsedTime = 0.f;

	/** Travel time (path length / speed) the pitch interpolation spans. */
	float MoveSoundTravelTime = 0.f;
};
