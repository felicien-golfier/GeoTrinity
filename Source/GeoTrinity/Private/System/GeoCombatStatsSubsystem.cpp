// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoCombatStatsSubsystem.h"

#include "Characters/PlayerClassTypes.h"
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
			SumEvents(Stats.DamageReceived) / RollingWindowSeconds);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoCombatStatsSubsystem::DisplayLines(AGeoPlayerState* LocalPlayerState) const
{
	if (!IsValid(LocalPlayerState))
	{
		return;
	}

	FString ClassName;
	FColor LineColor = FColor::White;
	switch (LocalPlayerState->GetPlayerClass())
	{
	case EPlayerClass::Triangle:
		ClassName = TEXT("Triangle");
		LineColor = FColor::Red;
		break;
	case EPlayerClass::Circle:
		ClassName = TEXT("Circle");
		LineColor = FColor::Green;
		break;
	case EPlayerClass::Square:
		ClassName = TEXT("Square");
		LineColor = FColor::Blue;
		break;
	default:
		ClassName = TEXT("Unknown");
		break;
	}

	FString const Line = FString::Printf(TEXT("[%s] %s — DPS: %.1f | HPS: %.1f | Recv: %.1f"),
										 *ClassName,
										 *LocalPlayerState->GetPlayerName(),
										 LocalPlayerState->GetDebugDPS(),
										 LocalPlayerState->GetDebugHPS(),
										 LocalPlayerState->GetDebugRecv());
	GEngine->AddOnScreenDebugMessage(-1, 0.f, LineColor, Line);
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
