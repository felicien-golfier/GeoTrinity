// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"

#include "GeoDeploySatelliteComponent.generated.h"

class UNiagaraSystem;
class UPrimitiveComponent;
class UStaticMesh;

/**
 * Look of one player class's deploy satellite ring. Lives in that class's FPlayerClassData, so each shape brings its
 * own satellites; UGeoDeploySatelliteComponent::SetParams pushes it in on every class change.
 */
USTRUCT(BlueprintType)
struct FSatelliteParams
{
	GENERATED_BODY()

	/** Mesh drawn for one available deploy charge, when no System is set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Niagara system played for one available deploy charge, in the mesh's place. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> System = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float OrbitRadius = 100.f;

	/** Orbit rate in degrees per second. Negative turns the other way. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float OrbitSpeed = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float Scale = 0.2f;

	/** Seconds a satellite takes to travel from the character's centre out to its slot on the ring. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float TravelTime = 1.f;
};

/**
 * Cosmetic ring of small meshes or Niagara systems orbiting a player, one per deploy charge still available.
 * Autonomous: each tick it reads the owner's deploy ability and adds or drops satellites to match its charges, so
 * nothing has to push a count at it. A new satellite appears at the character and reaches its slot in TravelTime
 * seconds while the others slide over to keep the spacing even.
 * Only the machine of the player owning the pawn shows the ring — deploy charges (cooldown GE stacks) reach no one
 * else.
 */
UCLASS(ClassGroup = "GeoTrinity", meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoDeploySatelliteComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Enables tick — the whole ring is tick-driven. */
	UGeoDeploySatelliteComponent();

	/**
	 * Consumes the satellite closest to the owner's facing so a shot can leave from it; the remaining ones re-space
	 * themselves on the next tick. Call at fire time.
	 *
	 * @param OutLaunchLocation  World location the consumed satellite was drawn at. Left untouched when none was up.
	 * @return True when a satellite was consumed.
	 */
	bool LaunchSatellite(FVector& OutLaunchLocation);

	/**
	 * Switches the ring to another class's look. Drops the satellites currently up so the tick rebuilds them with the
	 * new mesh — they fly back out from the character, which is what a class change should look like anyway.
	 *
	 * @param NewParams  The incoming class's FPlayerClassData satellite entry.
	 */
	void SetParams(FSatelliteParams const& NewParams);

	/** Matches the satellite count to the deploy charges, advances the orbit, and eases every satellite to its slot. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Satellites the ring should show: the charges left, plus the one a charging deploy has not launched yet. */
	int32 GetDesiredSatelliteCount() const;

	/** Creates one satellite (Niagara or mesh) at the character's centre; the tick flies it out to its slot. */
	void AddSatellite();

	/** Position of slot Index out of Count on the ring, relative to this component. */
	FVector GetSlotLocation(int32 Index, int32 Count) const;

	// Not edited here: the live values come from the current class's FPlayerClassData, pushed in by SetParams.
	UPROPERTY()
	FSatelliteParams Params;

	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> Satellites;

	float OrbitAngle = 0.f;
};
