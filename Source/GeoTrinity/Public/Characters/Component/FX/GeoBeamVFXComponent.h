// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoBeamVFXComponent.generated.h"

struct FGeoColorParam;
class UGeoZoneVisualComponent;

/** Replication bundle for the beam's visual dimensions and on/off state; a single OnRep fires when any field changes.
 */
USTRUCT()
struct FBeamVFXState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bActive = false;

	UPROPERTY()
	float HalfWidth = 0.f;

	UPROPERTY()
	float Length = 0.f;
};

/**
 * Replicated component dynamically added to a character for as long as a beam ability is granted
 * (OnGiveAbility/OnRemoveAbility) and toggled on/off via SetBeamState. The ability drives BeamState on the server;
 * replication delivers it to all clients, where each rendering machine draws it as a Beam on the zone layer through a
 * local UGeoZoneVisualComponent attached to the owner's root — so the beam turns with the character's aim.
 */
UCLASS()
class GEOTRINITY_API UGeoBeamVFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Enables component replication. */
	UGeoBeamVFXComponent();

	/** Creates the local zone shape on rendering machines and applies any already-replicated BeamState. */
	virtual void BeginPlay() override;
	/** Destroys the local zone shape before delegating to Super. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	/** Registers BeamState and ColorMask for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Shows or hides the beam and sets its size. The server's write replicates to all clients; the owning client may
	 * also call it for lag-free local visuals (its write only feeds its own shape). The windup telegraph is not this
	 * component's: it is a Ray on the owner's UGeoIndicatorComponent.
	 */
	void SetBeamState(bool bActive, float HalfWidth, float Length);
	/** Sets the beam's colours (GeoZoneLayer::MakeColorMask); call from the owning ability's OnGiveAbility. */
	void SetBeamColors(FGeoColorParam const& Color, TArray<FGeoColorParam> const& SecondaryColors);

private:
	UFUNCTION()
	void OnRep_BeamState() const;
	UFUNCTION()
	void OnRep_ColorMask() const;

	/** Pushes BeamState and ColorMask into the local zone shape. No-op on dedicated server. */
	void ApplyBeamState() const;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FBeamVFXState BeamState;

	/** One bit per EGeoColor ordinal. */
	UPROPERTY(ReplicatedUsing = OnRep_ColorMask)
	uint32 ColorMask = 0;

	/** This machine's drawing of the beam; null on a dedicated server. */
	UPROPERTY(Transient)
	TObjectPtr<UGeoZoneVisualComponent> ZoneVisual;
};
