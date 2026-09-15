// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoLagCompensatedEvent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void FGeoLagCompensatedEvent::Start(float const InEventServerTime, bool const bInSeenThroughReplication)
{
	EventServerTime = InEventServerTime;
	bSeenThroughReplication = bInSeenThroughReplication;
	JudgedActors.Reset();
}

TSet<AActor*> FGeoLagCompensatedEvent::JudgeActorsReachingEvent(TArray<AActor*> const& Candidates)
{
	TSet<AActor*> ActorsReachingEvent;
	for (AActor* Actor : Candidates)
	{
		if (!JudgedActors.Contains(Actor) && GeoLib::GetPerceivedServerTime(Actor) >= GetSeenServerTime(Actor))
		{
			JudgedActors.Add(Actor);
			ActorsReachingEvent.Add(Actor);
		}
	}
	return ActorsReachingEvent;
}

bool FGeoLagCompensatedEvent::IsOver(float const ServerTime) const
{
	float const MaxLatencyCompensation = GetDefault<UGameDataSettings>()->MaxLatencyCompensation;
	float const MaxSeenDelay = bSeenThroughReplication ? MaxLatencyCompensation : 0.f;
	return ServerTime >= EventServerTime + MaxSeenDelay + MaxLatencyCompensation;
}

float FGeoLagCompensatedEvent::GetSeenServerTime(AActor const* Actor) const
{
	APawn const* const Pawn = Cast<APawn>(Actor);
	if (!bSeenThroughReplication || !IsValid(Pawn) || !Pawn->IsPlayerControlled() || Pawn->IsLocallyControlled())
	{
		return EventServerTime;
	}

	float const HalfPing = Pawn->GetPlayerState()->GetPingInMilliseconds() * 0.0005f;
	return EventServerTime + FMath::Min(HalfPing, GetDefault<UGameDataSettings>()->MaxLatencyCompensation);
}
