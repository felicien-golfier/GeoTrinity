// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "GeoEncounterSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncounterEventRaised, FGeoEncounterPayload const& /*Payload, Tag filled*/);

/** Tag event bus of the encounters: an emitter raises an Encounter.Event.* tag, a GeoEventTrigger listens. It is the
 *  channel for an occurrence that has no delegate of its own to bind. */
UCLASS()
class GEOTRINITY_API UGeoEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Broadcasts Payload to every listener, with its Tag set to Tag. Callable from any emitter, Blueprint included. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter", meta = (GameplayTagFilter = "Encounter.Event"))
	void Raise(FGameplayTag Tag, FGeoEncounterPayload Payload);

	FOnEncounterEventRaised OnRaised;

	/** True while the encounters hold the first local player's movement lock (GeoSetMovementLockedAction): one lock in
	 * the controller's ignore-move count, however many lock actions ran. */
	bool bMovementLocked = false;
};
