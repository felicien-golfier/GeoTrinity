// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoCombatStatsSubsystem.h"

#include "Characters/PlayerClassTypes.h"
#include "GameFramework/GameStateBase.h"
#include "GeoPlayerState.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportDamageDealt(AGeoPlayerState* Source, float Amount)
{
	if (!IsValid(Source))
	{
		return;
	}
	float const CurrentTime = GetWorld()->GetTimeSeconds();
	RecordEvent(StatsPerActor.FindOrAdd(Source).DamageDealt, Amount, CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportDamageReceived(AGeoPlayerState* Target, float Amount)
{
	if (!IsValid(Target))
	{
		return;
	}
	float const CurrentTime = GetWorld()->GetTimeSeconds();
	RecordEvent(StatsPerActor.FindOrAdd(Target).DamageReceived, Amount, CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ReportHealingDealt(AGeoPlayerState* Source, float Amount)
{
	if (!IsValid(Source))
	{
		return;
	}
	float const CurrentTime = GetWorld()->GetTimeSeconds();
	RecordEvent(StatsPerActor.FindOrAdd(Source).HealingDealt, Amount, CurrentTime);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::ComputePlayerStats(float CurrentTime)
{
	// On authority, compute rolling averages and push them to each PlayerState so clients receive them via replication.
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

		GeoPlayerState->SetDebugCombatStats(SumEvents(Stats.DamageDealt) / RollingWindowSeconds,
											SumEvents(Stats.HealingDealt) / RollingWindowSeconds,
											SumEvents(Stats.DamageReceived) / RollingWindowSeconds);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
TArray<FString> UGeoCombatStatsSubsystem::BuildDisplayLines() const
{
	TArray<FString> Lines;

	AGameStateBase const* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		return Lines;
	}

	// Build display lines from replicated PlayerState data — works on both server and clients.
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AGeoPlayerState* GeoPlayerState = Cast<AGeoPlayerState>(PlayerState);
		if (!IsValid(GeoPlayerState))
		{
			continue;
		}

		FString ClassName;
		FColor LineColor;
		switch (GeoPlayerState->GetPlayerClass())
		{
		case EPlayerClass::Triangle:
			ClassName = TEXT("Triangle");
			LineColor = FColor::Red;
			break;
		case EPlayerClass::Circle:
			ClassName = TEXT("Circle");
			LineColor = FColor::Cyan;
			break;
		case EPlayerClass::Square:
			ClassName = TEXT("Square");
			LineColor = FColor::Blue;
			break;
		default:
			ClassName = TEXT("Unknown");
			LineColor = FColor::White;
			break;
		}

		FString const Line = FString::Printf(TEXT("[%s] %s — DPS: %.1f | HPS: %.1f | Recv: %.1f"), *ClassName,
											 *GeoPlayerState->GetPlayerName(), GeoPlayerState->GetDebugDPS(),
											 GeoPlayerState->GetDebugHPS(), GeoPlayerState->GetDebugRecv());
		Lines.Add(Line);
	}

	return Lines;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::RecordEvent(TArray<FCombatEventRecord>& Events, float Amount, float CurrentTime)
{
	Events.Add(FCombatEventRecord{CurrentTime, Amount});
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
