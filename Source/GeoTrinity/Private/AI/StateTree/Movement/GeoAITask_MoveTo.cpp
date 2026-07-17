// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/GeoAITask_MoveTo.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AIController.h"
#include "GameplayEffectTypes.h"
#include "NavigationData.h"

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
