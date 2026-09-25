// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployTargetCue.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "Actor/Projectile/GeoProjectile.h"

AGeoDeployTargetCue::AGeoDeployTargetCue()
{
	bAutoDestroyOnRemove = true;
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoDeployTargetCue::OnActive_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters)
{
	SetActorLocation(Parameters.Location);

	if (AGeoProjectile* LandingProjectile = Cast<AGeoProjectile>(MyTarget))
	{
		LandingProjectile->OnProjectileEndLifeDelegate.AddUniqueDynamic(this, &ThisClass::OnLandingProjectileEnded);
	}
	else
	{
		DeployAbility = Cast<UGeoDeployAbility>(Parameters.SourceObject.Get());
		ensureMsgf(DeployAbility.IsValid(),
				   TEXT("%s: a charging marker's SourceObject must be the UGeoDeployAbility instance."), *GetName());
		SetActorTickEnabled(true);
	}

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
void AGeoDeployTargetCue::OnLandingProjectileEnded(AGeoProjectile* Projectile)
{
	Projectile->OnProjectileEndLifeDelegate.RemoveDynamic(this, &ThisClass::OnLandingProjectileEnded);
	GameplayCueFinishedCallback();
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
