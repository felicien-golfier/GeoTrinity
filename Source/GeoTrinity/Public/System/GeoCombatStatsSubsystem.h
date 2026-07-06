// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GeoCombatStatsSubsystem.generated.h"

class AGeoPlayerState;

struct FActorCombatStats
{
	float SmoothedDPS = 0.f;
	float SmoothedHPS = 0.f;
	float BestDPS = 0.f;
	float BestHPS = 0.f;
	float TotalDamageDealt = 0.f;
	float TotalHealingDealt = 0.f;
	float TotalDamageReceived = 0.f;
};

/**
 * World subsystem that tracks per-player damage and healing and computes DPS / HPS (exponentially
 * smoothed current rate, best, and whole-combat average) for debug display. State is a fixed handful
 * of floats per player — no per-event storage. Stats reset when a fight starts (match InProgress);
 * when it ends they are dropped and the last pushed values stay displayed on the player states.
 * Server-only; not replicated.
 */
UCLASS()
class GEOTRINITY_API UGeoCombatStatsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Server-only: subscribes to match state changes so stats reset when a fight starts and are freed when it ends. */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Records an amount of damage dealt by Source and refreshes the displayed stats. */
	void ReportDamageDealt(AGeoPlayerState* Source, float Amount);
	/** Records an amount of damage received by Target and refreshes the displayed stats. Only the total is tracked. */
	void ReportDamageReceived(AGeoPlayerState* Target, float Amount);
	/** Records an amount of healing dealt by Source and refreshes the displayed stats. */
	void ReportHealingDealt(AGeoPlayerState* Source, float Amount);
	/**
	 * Decays the smoothed rates to CurrentTime and pushes updated stats to each player state.
	 * Ticked every frame; no-op while no combat session is running (StatsPerActor empty, e.g. right
	 * after a fight ends), which keeps the last fight's values frozen on the display.
	 */
	void ComputePlayerStats(float CurrentTime);

#if !UE_BUILD_SHIPPING
	/** Returns true when the in-game combat stats debug overlay should be rendered. */
	static bool IsDebugDisplayEnabled();
#endif

private:
	/**
	 * Time constant (seconds) of the exponential smoothing applied to the current DPS / HPS.
	 * Each event adds Amount / T to the rate and rates decay by e^(-dt/T), so steady damage reads as
	 * its true per-second rate while a lone burst fades out over roughly T seconds.
	 */
	static constexpr float SmoothingWindowSeconds = 3.f;

	TMap<TWeakObjectPtr<AGeoPlayerState>, FActorCombatStats> StatsPerActor;

	/** World time at which the current combat started. Whole-combat DPS/HPS averages are measured from here. */
	float CombatStartTime = 0.f;
	/** World time the smoothed rates were last decayed to. */
	float LastDecayTime = 0.f;

	/** Resets stats when a fight starts (InProgress) and drops all recorded stats when it ends. */
	UFUNCTION()
	void OnMatchStateChanged(FName MatchState, FName PreviousMatchState);
	/** Clears all recorded stats, restarts the combat timer, and pushes zeroed values to every tracked player state. */
	void ResetStats();

	/** Returns Actor's stats entry, creating it. The first event outside a match restarts the combat timer. */
	FActorCombatStats& FindOrAddStats(AGeoPlayerState* Actor, float CurrentTime);
	bool IsMatchInProgress() const;
	/** Exponentially decays every smoothed rate to CurrentTime, so a new event can be folded in undecayed. */
	void DecayRates(float CurrentTime);
	/** Updates the best rates and pushes all stats to the tracked player states. */
	void PushPlayerStats(float CurrentTime);
};
