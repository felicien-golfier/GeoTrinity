// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoCombatStatsSubsystem.h"

#include "GeoPlayerState.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<bool>
	CVarShowCombatStats(TEXT("Geo.ShowCombatStats"), true,
						TEXT("When true, shows per-player DPS / HPS / damage received on screen"));

bool UGeoCombatStatsSubsystem::IsDebugDisplayEnabled()
{
	return CVarShowCombatStats.GetValueOnGameThread();
}
#endif

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportDamageDealt(AGeoPlayerState* Source, float Amount)
{
	if (!IsValid(Source))
	{
		return;
	}
	float const CurrentTime = GetWorld()->GetTimeSeconds();
	FActorCombatStats& Stats = StatsPerActor.FindOrAdd(Source);
	RecordEvent(Stats.DamageDealt, Stats.TotalDamageDealt, Amount, CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportDamageReceived(AGeoPlayerState* Target, float Amount)
{
	if (!IsValid(Target))
	{
		return;
	}
	float const CurrentTime = GetWorld()->GetTimeSeconds();
	FActorCombatStats& Stats = StatsPerActor.FindOrAdd(Target);
	RecordEvent(Stats.DamageReceived, Stats.TotalDamageReceived, Amount, CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportHealingDealt(AGeoPlayerState* Source, float Amount)
{
	if (!IsValid(Source))
	{
		return;
	}
	float const CurrentTime = GetWorld()->GetTimeSeconds();
	FActorCombatStats& Stats = StatsPerActor.FindOrAdd(Source);
	RecordEvent(Stats.HealingDealt, Stats.TotalHealingDealt, Amount, CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ComputePlayerStats(float CurrentTime)
{
	for (auto It = StatsPerActor.CreateIterator(); It; ++It)
	{
		AGeoPlayerState* GeoPlayerState = It.Key().Get();
		if (!IsValid(GeoPlayerState))
		{
			It.RemoveCurrent();
			continue;
		}

		FActorCombatStats& Stats = It.Value();
		PruneEvents(Stats.DamageDealt, CurrentTime);
		PruneEvents(Stats.HealingDealt, CurrentTime);
		PruneEvents(Stats.DamageReceived, CurrentTime);

		GeoPlayerState->SetDebugCombatStats(
			SumEvents(Stats.DamageDealt) / RollingWindowSeconds,
			SumEvents(Stats.HealingDealt) / RollingWindowSeconds,
			SumEvents(Stats.DamageReceived) / RollingWindowSeconds,
			Stats.TotalDamageDealt,
			Stats.TotalHealingDealt,
			Stats.TotalDamageReceived);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::RecordEvent(TArray<FCombatEventRecord>& Events, float& Total, float Amount, float CurrentTime)
{
	Events.Add(FCombatEventRecord{CurrentTime, Amount});
	Total += Amount;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::PruneEvents(TArray<FCombatEventRecord>& Events, float CurrentTime)
{
	float const CutoffTime = CurrentTime - RollingWindowSeconds;
	Events.RemoveAll(
		[CutoffTime](FCombatEventRecord const& Record)
		{
			return Record.Timestamp < CutoffTime;
		});
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float UGeoCombatStatsSubsystem::SumEvents(TArray<FCombatEventRecord> const& Events)
{
	float Total = 0.f;
	for (FCombatEventRecord const& Record : Events)
	{
		Total += Record.Amount;
	}
	return Total;
}
