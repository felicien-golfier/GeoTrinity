// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "GeoEncounterSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncounterEventRaised, FGeoEncounterPayload const& /*Payload, Tag filled*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncounterCatLine, FText const& /*Line, tokens replaced*/);

/** Tag event bus of the encounters: an emitter raises an Encounter.Event.* tag, a GeoEventTrigger listens. It is the
 *  channel for an occurrence that has no delegate of its own to bind. */
UCLASS()
class GEOTRINITY_API UGeoEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Broadcasts Payload to every listener, with its Tag set to Tag. Callable from any emitter, Blueprint included. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter", meta = (GameplayTagFilter = "Encounter.Event"))
	void Raise(FGameplayTag Tag, FGeoEncounterPayload Payload);

	FOnEncounterEventRaised OnRaised;

	/** Broadcast by GeoCatLineAction when a Cat line shows, with or without a HUD. What tests read to know which line
	 * ran. */
	FOnEncounterCatLine OnCatLine;

	/** True while the encounters hold the first local player's movement lock (GeoSetMovementLockedAction): one lock in
	 * the controller's ignore-move count, however many lock actions ran. */
	bool bMovementLocked = false;

private:
	/** Aborts every encounter of the world, once: the session is ending, nothing may advance while it is torn down. */
	void AbortEncounters();

	FDelegateHandle WorldTearDownHandle;
#if WITH_EDITOR
	FDelegateHandle PIEEndedHandle;
#endif
	bool bSessionEnding = false;
};
