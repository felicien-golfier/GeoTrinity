// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoCombatStatsSubsystem.h"

#include "GameClasses/GeoGameState.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameMode.h"
#include "Tool/UGeoGameplayLibrary.h"

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
void UGeoCombatStatsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Server-only: stats are recorded and pushed from the server. No AGeoGameState in menu worlds — legitimate skip.
	AGeoGameState* GameState = InWorld.GetGameState<AGeoGameState>();
	if (GeoLib::IsServer(&InWorld) && GameState)
	{
		GameState->OnMatchStateChanged.AddUniqueDynamic(this, &UGeoCombatStatsSubsystem::OnMatchStateChanged);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::OnMatchStateChanged(FName MatchState, FName /*PreviousMatchState*/)
{
	if (MatchState == MatchState::InProgress)
	{
		ResetStats();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ResetStats()
{
	for (TPair<TWeakObjectPtr<AGeoPlayerState>, FActorCombatStats> const& Pair : StatsPerActor)
	{
		if (AGeoPlayerState* GeoPlayerState = Pair.Key.Get())
		{
			GeoPlayerState->SetDebugCombatStats(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
		}
	}
	StatsPerActor.Empty();
}

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
	StatsPerActor.FindOrAdd(Target).TotalDamageReceived += Amount;
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

		float const DPS = SumEvents(Stats.DamageDealt) / RollingWindowSeconds;
		float const HPS = SumEvents(Stats.HealingDealt) / RollingWindowSeconds;
		Stats.BestDPS = FMath::Max(Stats.BestDPS, DPS);
		Stats.BestHPS = FMath::Max(Stats.BestHPS, HPS);

		GeoPlayerState->SetDebugCombatStats(DPS, HPS, Stats.BestDPS, Stats.BestHPS, Stats.TotalDamageDealt,
											Stats.TotalHealingDealt, Stats.TotalDamageReceived);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::RecordEvent(TArray<FCombatEventRecord>& Events, float& Total, float Amount,
										   float CurrentTime)
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
