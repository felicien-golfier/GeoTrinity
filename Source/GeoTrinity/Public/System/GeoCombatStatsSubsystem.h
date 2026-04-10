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

UCLASS()
class GEOTRINITY_API UGeoCombatStatsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void ReportDamageDealt(AGeoPlayerState* Source, float Amount);
	void ReportDamageReceived(AGeoPlayerState* Target, float Amount);
	void ReportHealingDealt(AGeoPlayerState* Source, float Amount);
	void ComputePlayerStats(float CurrentTime);

#if !UE_BUILD_SHIPPING
	static bool IsDebugDisplayEnabled();
#endif

private:
	static constexpr float RollingWindowSeconds = 10.f;

	TMap<TWeakObjectPtr<AGeoPlayerState>, FActorCombatStats> StatsPerActor;

	void RecordEvent(TArray<FCombatEventRecord>& Events, float& Total, float Amount, float CurrentTime);
	static void PruneEvents(TArray<FCombatEventRecord>& Events, float CurrentTime);
	static float SumEvents(TArray<FCombatEventRecord> const& Events);
};
