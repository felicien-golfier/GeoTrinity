// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoBackgroundPulseComponent.generated.h"

class UMaterialParameterCollection;

UENUM()
enum class EGeoPulseMode : uint8
{
	/** Origins sit on the nearest characters and deployables inside AreaRadius. */
	Actors,
	/** Origins travel along a fixed heading, turning back on reaching AreaRadius. */
	Straight,
	/** Origins travel while their heading drifts at up to TurnRate degrees per second. */
	Wander, 
	None
};

/** One pulse's simulated state. Location and Direction go unused in Actors mode, where the origin is read off the
 * tracked actor each frame instead of integrated. */
struct FGeoPulse
{
	FVector2D Location = FVector2D::ZeroVector;
	FVector2D Direction = FVector2D(1.f, 0.f);
	float RingRadius = 0.f;
};

/**
 * Drives MPC_BackgroundPulse, the eight PulseSource_XX slots M_BackgroundLattice reads to light its triangle lines.
 * Each slot is (OriginX, OriginY, Radius, Intensity): a ring expands from its origin at RingSpeed, fading as it goes,
 * and restarts at MaxRingRadius. Mode decides only where the origins come from — the rings behave identically whether
 * they ride a player or wander on their own.
 *
 * Lives on AGeoGameCamera as a native subobject, so every radius it measures is taken from the camera's own position
 * and its area is always over the part of the level someone can see. That also makes the whole thing authorable on
 * BP_GeoCam with no spawning, no settings entry and no Blueprint of its own.
 *
 * Purely cosmetic and entirely local: nothing replicates and nothing is seeded, so two machines may wander
 * differently and no gameplay reads any of it. Disables its own tick on a dedicated server.
 */
UCLASS(ClassGroup = (Geo), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoBackgroundPulseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGeoBackgroundPulseComponent();

	/** Captures the authored Mode as the one ResetMode returns to, then seeds each pulse's position, heading and
	 * ring phase. */
	virtual void BeginPlay() override;

	/** Advances every ring, moves the origins per Mode, and writes the collection. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** The camera's pulse driver, or null when there is no camera — which is every dedicated server. */
	static UGeoBackgroundPulseComponent* Get(UObject const* WorldContextObject);

	/** Switches which behaviour drives the origins. Nothing is re-seeded: the pulses carry on from wherever the
	 * previous mode left them, so an arena taking over drifts its rings off the actors rather than teleporting them. */
	void SetMode(EGeoPulseMode NewMode) { Mode = NewMode; }
	/** Restores the Mode authored on the camera, which is what plays outside any fight. */
	void ResetMode() { Mode = AuthoredMode; }

protected:
	/** MPC_BackgroundPulse. Without it there is nothing to drive and the component disables its own tick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pulse")
	TObjectPtr<UMaterialParameterCollection> PulseCollection;

	/** What plays outside any fight; each AGeoArena overrides it for the duration of its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse")
	EGeoPulseMode Mode = EGeoPulseMode::Actors;

	/** Live pulses. The collection only has eight slots, so this is clamped to that on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "1", ClampMax = "8"))
	int32 PulseCount = 8;

	/** How fast a ring expands away from its origin, cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "1"))
	float RingSpeed = 900.f;

	/** Radius a ring has fully faded at and restarts from zero. With RingSpeed this is also the pulse period. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "1"))
	float MaxRingRadius = 1500.f;

	/** Region around the camera this covers: how far Actors mode looks, and the bound the moving modes turn at. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse", meta = (ClampMin = "1"))
	float AreaRadius = 2500.f;

	/** How fast an origin travels, cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse|Movement",
			  meta = (ClampMin = "0", EditCondition = "Mode != EGeoPulseMode::Actors", EditConditionHides))
	float MoveSpeed = 400.f;

	/** Largest heading change per second, in degrees. Zero makes Wander behave as Straight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulse|Movement",
			  meta = (ClampMin = "0", EditCondition = "Mode == EGeoPulseMode::Wander", EditConditionHides))
	float TurnRate = 120.f;

private:
	/** Characters and deployables within AreaRadius of the camera, nearest first. */
	TArray<AActor*> GatherTrackedActors() const;

	/** Advances Pulse along its heading, drifting that heading first in Wander mode, and turns it back toward the
	 * centre once it leaves AreaRadius. */
	void MovePulse(FGeoPulse& Pulse, float DeltaTime) const;

	/** Writes one PulseSource_XX slot. An all-zero value clears it: Intensity multiplies the ring in the shader, so
	 * a zeroed slot cancels itself and needs no out-of-range sentinel position. */
	void SetSlot(int32 SlotIndex, FLinearColor Value);

	/** Mode as authored on the camera, captured in BeginPlay. Kept rather than read back off a CDO because the
	 * authored value lives on BP_GeoCam's component template, which the native class default never sees. */
	EGeoPulseMode AuthoredMode = EGeoPulseMode::Actors;

	TArray<FGeoPulse> Pulses;
};
