// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"

class AActor;

/**
 * Server. Lag compensation for a hit landing at one moment (a zone expiring, a deployable exploding): each target is
 * judged once, the tick its own time (GeoLib::GetPerceivedServerTime) reaches the moment its screen showed the event,
 * so the server holds it where it stood then. See the lag compensation rule in AbilitySystem/Abilities/Pattern/CLAUDE.md.
 */
struct GEOTRINITY_API FGeoLagCompensatedEvent
{
	/**
	 * Forgets every target judged so far and starts judging a new event.
	 *
	 * @param InEventServerTime          Server time the event happened at.
	 * @param bInSeenThroughReplication  True when clients only learn about the event through replication (a deployable
	 *                                   exploding), so each one saw it half its ping late; false when they run it on the
	 *                                   server clock themselves (a pattern).
	 */
	void Start(float InEventServerTime, bool bInSeenThroughReplication);

	/** Returns the actors of Candidates whose own time has reached the event since the last call, each once per event.
	 * Pass every potential target, not only those inside the hit volume, then filter the result by the volume: a target
	 * judged outside it must not be hit by walking in later. */
	TSet<AActor*> JudgeActorsReachingEvent(TArray<AActor*> const& Candidates);

	/** True once every target has been judged — guaranteed after a fixed window, since GetPerceivedServerTime never
	 * trails the server by more than MaxLatencyCompensation. */
	bool IsOver(float ServerTime) const;

private:
	/** Server time Actor's screen showed the event at, the half ping capped at MaxLatencyCompensation. */
	float GetSeenServerTime(AActor const* Actor) const;

	float EventServerTime = 0.f;
	bool bSeenThroughReplication = false;
	TSet<TWeakObjectPtr<AActor>> JudgedActors;
};
