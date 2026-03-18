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

	TArray<FString> BuildDisplayLines() const;

private:
	static constexpr float RollingWindowSeconds = 30.f;

	TMap<TWeakObjectPtr<AGeoPlayerState>, FActorCombatStats> StatsPerActor;

	void RecordEvent(TArray<FCombatEventRecord>& Events, float Amount, float CurrentTime);
	static void PruneEvents(TArray<FCombatEventRecord>& Events, float CurrentTime);
	static float SumEvents(TArray<FCombatEventRecord> const& Events);
};
