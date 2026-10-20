// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoPlayerIdleTrigger.h"

#include "Characters/GeoCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void UGeoPlayerIdleTrigger::OnArm_Implementation()
{
	APlayerController const* const PlayerController = GetWorld()->GetFirstPlayerController();
	APawn const* const Player = PlayerController ? PlayerController->GetPawn() : nullptr;
	StillLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	StillSeconds = 0.f;
	Tier = 0;
	GetWorld()->GetTimerManager().SetTimer(PollTimer, this, &UGeoPlayerIdleTrigger::Poll, PollSeconds, true);
}

void UGeoPlayerIdleTrigger::OnDisarm_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(PollTimer);
}

void UGeoPlayerIdleTrigger::Poll()
{
	APlayerController const* const PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* const Player = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (Player)
	{
		AGeoCharacter const* const Character = Cast<AGeoCharacter>(Player);
		FVector const Location = Player->GetActorLocation();
		bool const bStill = !(Character && Character->IsDead()) && FVector::Dist2D(Location, StillLocation) <= Radius;
		StillSeconds = bStill ? StillSeconds + PollSeconds : 0.f;
		Tier = bStill ? Tier : 0;
		if (!bStill)
		{
			StillLocation = Location;
		}

		if (StillSeconds >= Delay)
		{
			StillSeconds = 0.f;
			StillLocation = Location;
			FGeoEncounterPayload Payload;
			Payload.Actor = Player;
			Payload.Count = ++Tier;
			Fire(Payload);
		}
	}
}
