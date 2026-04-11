// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GeoCombatStatsSubsystem.generated.h"

class AGeoPlayerState;

/** A single timestamped combat event (damage or healing), used for rolling-window DPS/HPS calculations. */
struct FCombatEventRecord
{
	float Timestamp;
	float Amount;
};

/** Aggregated combat statistics for one player: recent events in a rolling window plus running totals. */
struct FActorCombatStats
{
	TArray<FCombatEventRecord> DamageDealt;
	TArray<FCombatEventRecord> HealingDealt;
	TArray<FCombatEventRecord> DamageReceived;
	float TotalDamageDealt = 0.f;
	float TotalHealingDealt = 0.f;
	float TotalDamageReceived = 0.f;
};

/**
 * World subsystem that tracks per-player combat statistics (DPS, HPS, damage received) in a rolling window.
 * Other systems call the Report* functions; AGeoPlayerController calls ComputePlayerStats each frame (non-shipping)
 * to push the computed values back to each AGeoPlayerState for HUD display.
 */
UCLASS()
class GEOTRINITY_API UGeoCombatStatsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Records an outgoing damage event for Source. Call from ExecCalc_Damage after the hit is applied. */
	void ReportDamageDealt(AGeoPlayerState* Source, float Amount);
	/** Records an incoming damage event for Target. Call from ExecCalc_Damage after the hit is applied. */
	void ReportDamageReceived(AGeoPlayerState* Target, float Amount);
	/** Records an outgoing heal event for Source. Call whenever a heal GE is successfully applied. */
	void ReportHealingDealt(AGeoPlayerState* Source, float Amount);
	/**
	 * Prunes stale events outside the rolling window and pushes updated DPS/HPS values to each player state.
	 *
	 * @param CurrentTime  Current world time in seconds (GetWorld()->GetTimeSeconds()).
	 */
	void ComputePlayerStats(float CurrentTime);

#if !UE_BUILD_SHIPPING
	/** Returns true when the combat stats debug overlay should be drawn (enabled via console variable). */
	static bool IsDebugDisplayEnabled();
#endif

private:
	static constexpr float RollingWindowSeconds = 10.f;

	TMap<TWeakObjectPtr<AGeoPlayerState>, FActorCombatStats> StatsPerActor;

	void RecordEvent(TArray<FCombatEventRecord>& Events, float& Total, float Amount, float CurrentTime);
	static void PruneEvents(TArray<FCombatEventRecord>& Events, float CurrentTime);
	static float SumEvents(TArray<FCombatEventRecord> const& Events);
};
