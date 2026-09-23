// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployTargetCue.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

AGeoDeployTargetCue::AGeoDeployTargetCue()
{
	bAutoDestroyOnRemove = true;
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoDeployTargetCue::OnActive_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters)
{
	DeployAbility = Cast<UGeoDeployAbility>(Parameters.SourceObject.Get());
	if (!ensureMsgf(DeployAbility.IsValid(), TEXT("%s: SourceObject must be the UGeoDeployAbility instance."),
					*GetName()))
	{
		return false;
	}

	SetActorLocation(Parameters.Location);
	SetActorTickEnabled(true);
	return Super::OnActive_Implementation(MyTarget, Parameters);
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoDeployTargetCue::OnRemove_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters)
{
	SetActorTickEnabled(false);
	DeployAbility.Reset();
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoDeployTargetCue::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (UGeoDeployAbility const* Ability = DeployAbility.Get())
	{
		SetActorLocation(Ability->GetPendingDeployLocation());
	}
}
