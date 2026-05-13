// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoDetonateAllMinesAbility.h"

#include "Actor/Deployable/Mine/GeoMine.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "GameFramework/PlayerState.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDetonateAllMinesAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	AActor* PayloadOwner = StoredPayload.Owner;
	APawn* Pawn = Cast<APawn>(PayloadOwner);
	if (!Pawn)
	{
		if (APlayerState const* PlayerState = Cast<APlayerState>(PayloadOwner))
		{
			Pawn = PlayerState->GetPawn();
		}
	}

	if (!ensureMsgf(Pawn, TEXT("UGeoDetonateAllMinesAbility: no valid Pawn")))
	{
		EndAbility(false, true);
		return;
	}

	UGeoDeployableManagerComponent* DeployableManager = Pawn->GetComponentByClass<UGeoDeployableManagerComponent>();
	if (!ensureMsgf(DeployableManager, TEXT("UGeoDetonateAllMinesAbility: no UGeoDeployableManagerComponent on '%s'"),
					*GetNameSafe(Pawn)))
	{
		EndAbility(false, true);
		return;
	}

	for (AGeoDeployableBase* Deployable : TArray(DeployableManager->GetDeployables()))
	{
		if (AGeoMine* Mine = Cast<AGeoMine>(Deployable); Mine && Mine->IsActive())
		{
			Mine->Recall(DetonationMultiplier);
		}
	}

	EndAbility(GeoLib::IsServer(GetWorld()), false);
}
