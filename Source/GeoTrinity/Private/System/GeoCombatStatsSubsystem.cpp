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
void UGeoCombatStatsSubsystem::OnMatchStateChanged(FName MatchState, FName PreviousMatchState)
{
	if (MatchState == MatchState::InProgress)
	{
		ResetStats();
	}
	else if (PreviousMatchState == MatchState::InProgress)
	{
		// Fight over: drop all per-player stats. Player states keep their last pushed values so the
		// HUD still shows the final fight stats.
		StatsPerActor.Empty();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ResetStats()
{
	// Zero every player's displayed stats, not just tracked ones: a new session must wipe the frozen
	// values of the previous fight for everyone.
	if (AGameState const* GameState = GetWorld()->GetGameState<AGameState>())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (AGeoPlayerState* GeoPlayerState = Cast<AGeoPlayerState>(PlayerState))
			{
				GeoPlayerState->SetDebugCombatStats({});
			}
		}
	}
	StatsPerActor.Empty();
	CombatStartTime = GetWorld()->GetTimeSeconds();
	LastDecayTime = CombatStartTime;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
FActorCombatStats& UGeoCombatStatsSubsystem::FindOrAddStats(AGeoPlayerState* Actor)
{
	// First event outside a match (e.g. training dummy) opens a new combat session, resetting values
	// like the InProgress transition does; during a match the session runs from that transition.
	if (StatsPerActor.IsEmpty() && !IsMatchInProgress())
	{
		ResetStats();
	}
	return StatsPerActor.FindOrAdd(Actor);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
bool UGeoCombatStatsSubsystem::IsMatchInProgress() const
{
	AGameState const* GameState = GetWorld()->GetGameState<AGameState>();
	return GameState && GameState->IsMatchInProgress();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportRate(AGeoPlayerState* Source, float Amount, FCombatRate FActorCombatStats::* Side)
{
	if (!IsValid(Source))
	{
		return;
	}
	float const CurrentTime = GetWorld()->GetTimeSeconds();
	DecayRates(CurrentTime);
	FCombatRate& Rate = FindOrAddStats(Source).*Side;
	Rate.Total += Amount;
	Rate.Smoothed += Amount / SmoothingWindowSeconds;
	Rate.Burst.Add(Amount, CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportDamageDealt(AGeoPlayerState* Source, float Amount)
{
	ReportRate(Source, Amount, &FActorCombatStats::Damage);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportDamageReceived(AGeoPlayerState* Target, float Amount)
{
	if (!IsValid(Target))
	{
		return;
	}
	DecayRates(GetWorld()->GetTimeSeconds());
	FindOrAddStats(Target).TotalDamageReceived += Amount;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportHealingDealt(AGeoPlayerState* Source, float Amount)
{
	ReportRate(Source, Amount, &FActorCombatStats::Healing);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ComputePlayerStats(float CurrentTime)
{
	// Empty means no combat session is running (e.g. a fight just ended): keep the last displayed
	// values frozen instead of decaying them.
	if (StatsPerActor.IsEmpty())
	{
		return;
	}
	DecayRates(CurrentTime);
	PushPlayerStats(CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::DecayRates(float CurrentTime)
{
	// Several reports can land in the same frame at an identical world time; decaying by zero elapsed
	// time is the identity, so skip the Exp and the map walk.
	if (CurrentTime == LastDecayTime)
	{
		return;
	}
	float const Decay = FMath::Exp((LastDecayTime - CurrentTime) / SmoothingWindowSeconds);
	LastDecayTime = CurrentTime;
	for (TPair<TWeakObjectPtr<AGeoPlayerState>, FActorCombatStats>& Pair : StatsPerActor)
	{
		Pair.Value.Damage.Smoothed *= Decay;
		Pair.Value.Healing.Smoothed *= Decay;
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::PushPlayerStats(float CurrentTime)
{
	// Clamped to one second so the first hits of a combat don't read as an absurd average.
	float const CombatDuration = FMath::Max(CurrentTime - CombatStartTime, 1.f);
	for (auto It = StatsPerActor.CreateIterator(); It; ++It)
	{
		AGeoPlayerState* GeoPlayerState = It.Key().Get();
		if (!IsValid(GeoPlayerState))
		{
			It.RemoveCurrent();
			continue;
		}

		FActorCombatStats const& Stats = It.Value();
		FGeoCombatDisplayStats Display;
		Display.DebugDPS = Stats.Damage.Smoothed;
		Display.DebugHPS = Stats.Healing.Smoothed;
		Display.MaxBurstDamage = Stats.Damage.Burst.Max;
		Display.MaxBurstHealing = Stats.Healing.Burst.Max;
		Display.FightDPS = Stats.Damage.Total / CombatDuration;
		Display.FightHPS = Stats.Healing.Total / CombatDuration;
		Display.TotalDamageDealt = Stats.Damage.Total;
		Display.TotalHealingDealt = Stats.Healing.Total;
		Display.TotalDamageReceived = Stats.TotalDamageReceived;
		GeoPlayerState->SetDebugCombatStats(Display);
	}
}
