// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoArenaBarrier.h"

#include "GameClasses/GeoGameState.h"
#include "Net/UnrealNetwork.h"

AGeoArenaBarrier::AGeoArenaBarrier()
{
	bReplicates = true;
}

void AGeoArenaBarrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoArenaBarrier, bIsClosed);
}
void AGeoArenaBarrier::BeginPlay()
{
	Super::BeginPlay();
	TickLerp();
}

void AGeoArenaBarrier::SetClosed(bool bNewClosed)
{
	bIsClosed = bNewClosed;
	OnRep_bIsClosed();
	OnBarrierStateChanged(bIsClosed);
}

void AGeoArenaBarrier::OnBarrierStateChanged_Implementation(bool bClosed)
{
	TickLerp();
}

// void AGeoArenaBarrier::CaptureFightOnLocations()
// {
// 	for (FBarrierAnimatedActor& AnimatedActor : AnimatedActors)
// 	{
// 		if (AnimatedActor.Actor)
// 		{
// 			AnimatedActor.FightOnTransform = AnimatedActor.Actor->GetActorTransform();
// 		}
// 	}
// }
//
// void AGeoArenaBarrier::CaptureFightOffLocations()
// {
// 	for (FBarrierAnimatedActor& AnimatedActor : AnimatedActors)
// 	{
// 		if (AnimatedActor.Actor)
// 		{
// 			AnimatedActor.FightOffTransform = AnimatedActor.Actor->GetActorTransform();
// 		}
// 	}
// }

void AGeoArenaBarrier::OnRep_bIsClosed()
{
	OnBarrierStateChanged(bIsClosed);
}

void AGeoArenaBarrier::TickLerp()
{
	float LerpDuration = GetWorld()->GetGameStateChecked<AGeoGameState>()->CommitFightTime;
	float const Step = LerpDuration > 0.0f ? GetWorld()->GetDeltaSeconds() / LerpDuration : 1.0f;
	LerpAlpha = FMath::Clamp(LerpAlpha + (bIsClosed ? Step : -Step), 0.0f, 1.0f);

	for (FBarrierAnimatedActor const& AnimatedActor : AnimatedActors)
	{
		if (!AnimatedActor.Actor)
		{
			ensureMsgf(AnimatedActor.Actor, TEXT("AGeoArenaBarrier: AnimatedActors entry has no Actor assigned."));
			continue;
		}

		float const Alpha = AnimatedActor.bHasMovement ? LerpAlpha : static_cast<float>(!bIsClosed);
		FTransform LerpedTransform;
		LerpedTransform.Blend(AnimatedActor.FightOffTransform, AnimatedActor.FightOnTransform, Alpha);
		AnimatedActor.Actor->SetActorTransform(LerpedTransform);
	}

	if ((bIsClosed && LerpAlpha <= 1.0f) || (!bIsClosed && LerpAlpha >= 0.0f))
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AGeoArenaBarrier::TickLerp);
	}
}
