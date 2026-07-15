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

namespace
{
	constexpr float MoveSoundPitchUpdateInterval = 0.05f;
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
void UGeoAITask_MoveTo::OnDestroy(bool bInOwnerFinished)
{
	StopMoveSound();
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

	MoveSoundComponent = UGameplayStatics::SpawnSoundAttached(MoveSound, Character->GetRootComponent(), NAME_None,
	                                                          FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
	                                                          true, 1.f, GetMoveSoundPitch(0.f));
	if (!MoveSoundComponent)
	{
		return;
	}

	MoveSoundStartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(MoveSoundTimerHandle, this, &UGeoAITask_MoveTo::UpdateMoveSoundPitch,
	                                        MoveSoundPitchUpdateInterval, true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::UpdateMoveSoundPitch()
{
	if (!MoveSoundComponent)
	{
		StopMoveSound();
		return;
	}

	float const Alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - MoveSoundStartTime) / MoveSoundTravelTime, 0.f, 1.f);
	MoveSoundComponent->SetPitchMultiplier(GetMoveSoundPitch(Alpha));

	// Pitch has reached the curve's end; hold it there and stop ticking. The loop plays on until the move actually
	// finishes (OnDestroy → StopMoveSound), so a move that overruns the length/speed estimate stays audible.
	if (Alpha >= 1.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(MoveSoundTimerHandle);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoAITask_MoveTo::GetMoveSoundPitch(float Alpha) const
{
	return PitchCurve ? PitchCurve->GetFloatValue(Alpha) : 1.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::StopMoveSound()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MoveSoundTimerHandle);
	}

	if (MoveSoundComponent)
	{
		MoveSoundComponent->Stop();
		MoveSoundComponent = nullptr;
	}
}
