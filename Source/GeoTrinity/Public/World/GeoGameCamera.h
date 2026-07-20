// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoGameCamera.generated.h"

class AGeoCharacter;

/**
 * Orthographic follow camera for GeoTrinity.
 * Always follows the local player with exponential smoothing — no edge-trigger dead zone.
 * World-space bounds are computed at BeginPlay from the AGeoTargetPoint actors tagged TargetPoint.CameraBounds
 * for the arena currently being played.
 * Camera decelerates naturally at borders because the clamped target shrinks the interp gap.
 * Z position is fixed to the actor's initial spawn height.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API AGeoGameCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AGeoGameCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	/** Exponential follow speed. Higher = snappier. Typical range 2–8. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.1"))
	float FollowInterpSpeed = 5.f;

	/** Free-camera pan speed (units/s) while spectating (local player dead). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.0"))
	float SpectateMoveSpeed = 1500.f;

private:
	/** Binds OnActiveArenaChanged and runs the initial bounds calc once the replicated GameState exists. On a late
	 * joiner the GameState may not have replicated by BeginPlay, so this retries on the next tick until it is valid. */
	void TryBindToGameState();
	/** Recomputes the bounds from the arena the players are currently in. */
	UFUNCTION()
	void CalculateBounds();
	/** Frames the camera on ArenaTag's TargetPoint.CameraBounds points. Keeps the previous bounds when none match. */
	void SetArenaTag(FGameplayTag ArenaTag);

	/** Reads the move-action value straight from the Enhanced Input player subsystem — the dead pawn's input
	 * component is disabled, so the pawn's own move callback never fires while spectating. */
	FVector2D GetSpectateMoveInput(APlayerController const* PlayerController,
								   AGeoCharacter const* LocalCharacter) const;

	FBox2D Bounds{};
	/** Free-camera target while the local player is dead; driven by move input, clamped to Bounds each tick. */
	FVector2D SpectateTarget = FVector2D::ZeroVector;
	bool bSpectating = false;
};
