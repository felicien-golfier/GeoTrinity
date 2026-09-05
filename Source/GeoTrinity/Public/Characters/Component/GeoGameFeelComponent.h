// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/Component/GeoFXComponent.h"
#include "CoreMinimal.h"

#include "GeoGameFeelComponent.generated.h"

class UMeshComponent;

/**
 * Cosmetic reactions of an actor that draws itself with a mesh: hit flash, recoil, and the cue rate limit that keeps a
 * per-tick effect from firing one cue per frame. Adds them to the VFX and sound playback it inherits, which is what
 * shows this owner's buffs.
 * Add to AGeoCharacter and AGeoInteractableActor subclasses. Auto-discovers the owner's first mesh on BeginPlay.
 */
UCLASS(ClassGroup = "GeoTrinity", meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoGameFeelComponent : public UGeoFXComponent
{
	GENERATED_BODY()

public:
	/** Enables tick and initializes default values. */
	UGeoGameFeelComponent();

	/** Discovers and caches the owner's first mesh for hit-flash and recoil application. */
	virtual void BeginPlay() override;
	/** Springs the recoil offset back toward the mesh's resting position. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** Flashes the owner's mesh with HitFlashMaterial for HitFlashDuration seconds. Uses LocalPlayerHitFlashMaterial
	 * when owner is the local player. */
	void FlashOnHit();

	/**
	 * Kicks the mesh backward opposite to Yaw by Distance cm, then springs back automatically.
	 * No-op on a pawn this machine doesn't control, whose mesh relative location belongs to network smoothing.
	 *
	 * @param Distance  How far the mesh snaps back in cm.
	 */
	void ApplyRecoil(float Distance);

	/**
	 * Returns true when enough time has passed since the last heal (bIsHeal) or damage GameplayCue to fire a new one.
	 * Records the current time on success so subsequent calls within the rate window return false.
	 * Call server-side only — the rate state is not replicated.
	 */
	bool IsCueAvailable(bool bIsHeal);

	UPROPERTY(EditDefaultsOnly, Category = "GeoGameFeel", meta = (ClampMin = "0"))
	float RecoilRecoverySpeed = 14.f;

private:
	UPROPERTY()
	TObjectPtr<UMeshComponent> TargetMesh;

	FTimerHandle HitFlashTimerHandle;

	double LastDamageCueTime = 0.0;
	double LastHealCueTime = 0.0;

	FVector CurrentRecoilOffset = FVector::ZeroVector;
	FVector InitialMeshRelativeLocation = FVector::ZeroVector;
};
