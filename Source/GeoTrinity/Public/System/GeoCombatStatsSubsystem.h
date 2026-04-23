// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GeoCombatStatsSubsystem.generated.h"

class AGeoPlayerState;

struct FCombatEventRecord
{
	float Timestamp;
	float Amount;
};

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
 * World subsystem that records per-player damage and healing events in a rolling window
 * and computes DPS / HPS / recv for debug display. Server-only; not replicated.
 */
UCLASS()
class GEOTRINITY_API UGeoCombatStatsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Records an amount of damage dealt by Source. Used to compute DPS over the rolling window. */
	void ReportDamageDealt(AGeoPlayerState* Source, float Amount);
	/** Records an amount of damage received by Target. Used to compute recv over the rolling window. */
	void ReportDamageReceived(AGeoPlayerState* Target, float Amount);
	/** Records an amount of healing dealt by Source. Used to compute HPS over the rolling window. */
	void ReportHealingDealt(AGeoPlayerState* Source, float Amount);
	/** Prunes expired events and pushes updated DPS/HPS/recv values to each player state. Call once per tick. */
	void ComputePlayerStats(float CurrentTime);

#if !UE_BUILD_SHIPPING
	/** Returns true when the in-game combat stats debug overlay should be rendered. */
	static bool IsDebugDisplayEnabled();
#endif

private:
	static constexpr float RollingWindowSeconds = 10.f;

	TMap<TWeakObjectPtr<AGeoPlayerState>, FActorCombatStats> StatsPerActor;

	void RecordEvent(TArray<FCombatEventRecord>& Events, float& Total, float Amount, float CurrentTime);
	static void PruneEvents(TArray<FCombatEventRecord>& Events, float CurrentTime);
	static float SumEvents(TArray<FCombatEventRecord> const& Events);
};
