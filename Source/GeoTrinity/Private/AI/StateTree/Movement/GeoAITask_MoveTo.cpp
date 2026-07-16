// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/GeoAITask_MoveTo.h"

#include "AIController.h"
#include "Components/AudioComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationData.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoAITask_MoveTo::UGeoAITask_MoveTo(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// UAITask_MoveTo::ResetTimers clears every timer bound to this task on each PerformMove, so the pitch cannot be
	// driven from a world timer.
	bTickingTask = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::PerformMove()
{
	Super::PerformMove();

	if (Path.IsValid())
	{
		Path->EnableRecalculationOnInvalidation(true);
		PlayMoveSound();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!MoveSoundComponent)
	{
		return;
	}

	MoveSoundElapsedTime += DeltaTime;
	float const Alpha = FMath::Clamp(MoveSoundElapsedTime / MoveSoundTravelTime, 0.f, 1.f);
	MoveSoundComponent->SetPitchMultiplier(GetMoveSoundPitch(Alpha));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::OnDestroy(bool bInOwnerFinished)
{
	if (MoveSoundComponent)
	{
		MoveSoundComponent->Stop();
		MoveSoundComponent = nullptr;
	}

	PlayEndSound();

	Super::OnDestroy(bInOwnerFinished);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::PlayMoveSound()
{
	// PerformMove re-runs on every path replan; keep the single loop started at the move's start playing across them.
	if (MoveSoundComponent || !MoveSound || GeoLib::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	AAIController* Controller = GetAIController();
	ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	if (!Character)
	{
		return;
	}

	float const MoveSpeed = Character->GetCharacterMovement()->GetMaxSpeed();
	if (MoveSpeed <= 0.f)
	{
		return;
	}

	MoveSoundTravelTime = Path->GetLength() / MoveSpeed;
	if (MoveSoundTravelTime <= 0.f)
	{
		return;
	}

	MoveSoundComponent =
		UGameplayStatics::SpawnSoundAttached(MoveSound, Character->GetRootComponent(), NAME_None, FVector::ZeroVector,
											 EAttachLocation::KeepRelativeOffset, true, 1.f, GetMoveSoundPitch(0.f));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::PlayEndSound()
{
	AAIController* Controller = GetAIController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!EndSound || !Pawn || GeoLib::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, EndSound, Pawn->GetActorLocation());
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoAITask_MoveTo::GetMoveSoundPitch(float Alpha) const
{
	return PitchCurve ? PitchCurve->GetFloatValue(Alpha) : 1.f;
}
