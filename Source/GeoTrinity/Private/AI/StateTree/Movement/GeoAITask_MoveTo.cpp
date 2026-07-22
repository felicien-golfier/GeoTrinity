// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/GeoAITask_MoveTo.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AIController.h"
#include "Characters/GeoCharacter.h"
#include "GameplayEffectTypes.h"
#include "NavigationData.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoAITask_MoveTo::UGeoAITask_MoveTo(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bTickingTask = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::PerformMove()
{
	Super::PerformMove();

	if (Path.IsValid())
	{
		Path->EnableRecalculationOnInvalidation(true);
		AddMoveGameplayCue();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::OnDestroy(bool bInOwnerFinished)
{
	if (MoveCueASC.IsValid())
	{
		MoveCueASC->RemoveGameplayCue(MoveGameplayCueTag);
		MoveCueASC.Reset();
	}

	Super::OnDestroy(bInOwnerFinished);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::TickTask(float const DeltaTime)
{
	Super::TickTask(DeltaTime);

	AAIController* Controller = GetAIController();
	AGeoCharacter* GeoPawn = Controller ? Cast<AGeoCharacter>(Controller->GetPawn()) : nullptr;
	if (!GeoPawn)
	{
		return;
	}

	FVector2D const Velocity2D(GeoPawn->GetVelocity());
	if (Velocity2D.SizeSquared() > FMath::Square(KINDA_SMALL_NUMBER))
	{
		GeoPawn->SetTargetYaw(FMath::Atan2(Velocity2D.Y, Velocity2D.X) * (180.f / PI));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::AddMoveGameplayCue()
{
	if (MoveCueASC.IsValid() || !MoveGameplayCueTag.IsValid())
	{
		return;
	}

	AAIController* Controller = GetAIController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UGeoAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(Pawn);
	if (!ensureMsgf(ASC, TEXT("UGeoAITask_MoveTo: %s needs an ASC to add the move cue %s"), *GetNameSafe(Pawn),
					*MoveGameplayCueTag.ToString()))
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.RawMagnitude = static_cast<float>(Path->GetLength());
	ASC->AddGameplayCue(MoveGameplayCueTag, CueParameters);
	MoveCueASC = ASC;
}
