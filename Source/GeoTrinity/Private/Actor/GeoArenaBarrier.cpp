// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoArenaBarrier.h"

#include "GameClasses/GeoGameState.h"
#include "Net/UnrealNetwork.h"

AGeoArenaBarrier::AGeoArenaBarrier()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AGeoArenaBarrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoArenaBarrier, bIsClosed);
}
void AGeoArenaBarrier::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);
}

void AGeoArenaBarrier::SetClosed(bool bNewClosed)
{
	bIsClosed = bNewClosed;
	OnRep_bIsClosed();
}

void AGeoArenaBarrier::CaptureFightOnTransforms()
{
	for (FBarrierAnimatedActor& AnimatedActor : AnimatedActors)
	{
		if (AnimatedActor.Actor)
		{
			AnimatedActor.FightOnTransform = AnimatedActor.Actor->GetActorTransform();
		}
	}
}

void AGeoArenaBarrier::CaptureFightOffTransforms()
{
	for (FBarrierAnimatedActor& AnimatedActor : AnimatedActors)
	{
		if (AnimatedActor.Actor)
		{
			AnimatedActor.FightOffTransform = AnimatedActor.Actor->GetActorTransform();
		}
	}
}

void AGeoArenaBarrier::SetToFightOnTransforms()
{
	for (FBarrierAnimatedActor const& AnimatedActor : AnimatedActors)
	{
		if (AnimatedActor.Actor)
		{
			AnimatedActor.Actor->SetActorTransform(AnimatedActor.FightOnTransform);
		}
	}
}

void AGeoArenaBarrier::SetToFightOffTransforms()
{
	for (FBarrierAnimatedActor const& AnimatedActor : AnimatedActors)
	{
		if (AnimatedActor.Actor)
		{
			AnimatedActor.Actor->SetActorTransform(AnimatedActor.FightOffTransform);
		}
	}
}

void AGeoArenaBarrier::OnRep_bIsClosed()
{
	SetActorTickEnabled(true);
	OnBarrierStateChanged(bIsClosed);
}

void AGeoArenaBarrier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AGeoGameState const* GameState = GetWorld()->GetGameStateChecked<AGeoGameState>();
	float const LerpDuration = bIsClosed ? GameState->CommitFightTime : GameState->DeathTime;
	float const Step = LerpDuration > 0.0f ? DeltaSeconds / LerpDuration : 1.0f;
	LerpAlpha = FMath::Clamp(LerpAlpha + (bIsClosed ? Step : -Step), 0.0f, 1.0f);

	for (FBarrierAnimatedActor const& AnimatedActor : AnimatedActors)
	{
		if (!ensureMsgf(AnimatedActor.Actor, TEXT("AGeoArenaBarrier: AnimatedActors entry has no Actor assigned.")))
		{
			continue;
		}

		float const Alpha = AnimatedActor.bHasMovement ? LerpAlpha : static_cast<float>(!bIsClosed);
		FTransform LerpedTransform;
		LerpedTransform.Blend(AnimatedActor.FightOffTransform, AnimatedActor.FightOnTransform, Alpha);
		AnimatedActor.Actor->SetActorTransform(LerpedTransform);
	}

	if (bIsClosed ? LerpAlpha >= 1.0f : LerpAlpha <= 0.0f)
	{
		SetActorTickEnabled(false);
	}
}
