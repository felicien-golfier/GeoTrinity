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
 * speeds up / slows down with the distance travelled without changing the clip.
 */
UCLASS()
class GEOTRINITY_API UGeoAITask_MoveTo : public UAITask_MoveTo
{
	GENERATED_BODY()

public:
	/** Looping sound played for the duration of a move. Its pitch is driven by PitchCurve over the move. Set by FSTTask_MoveTo. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> MoveSound;

	/** Pitch multiplier for MoveSound sampled at the 0..1 move progress (X=0 start, X=1 arrival). Set by FSTTask_MoveTo. */
	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> PitchCurve;

protected:
	virtual void PerformMove() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	/** Starts MoveSound as a loop on the pawn at the move's start and drives its pitch over the path's travel time.
	 *  No-op if a loop is already playing (path replans re-enter here) or on a dedicated server. */
	void PlayMoveSound();

	/** Timer callback: updates the loop's pitch from move progress, stopping the sound once the travel time elapses. */
	void UpdateMoveSoundPitch();

	/** Pitch multiplier for the given 0..1 move progress: PitchCurve sampled at Alpha, or 1 when no curve is set. */
	float GetMoveSoundPitch(float Alpha) const;

	/** Stops the move sound loop and clears the pitch-update timer. Safe to call when nothing is playing. */
	void StopMoveSound();

	/** Active move-sound loop, kept alive across pitch updates; null when no move sound is playing. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MoveSoundComponent;

	/** World time the current move sound started, used to compute 0..1 progress against MoveSoundTravelTime. */
	float MoveSoundStartTime = 0.f;

	/** Travel time (path length / speed) the pitch interpolation spans. */
	float MoveSoundTravelTime = 0.f;

	/** Handle for the repeating pitch-update timer. */
	FTimerHandle MoveSoundTimerHandle;
};
