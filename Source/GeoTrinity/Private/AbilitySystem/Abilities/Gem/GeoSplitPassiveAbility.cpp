// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoSplitPassiveAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoSplitPassiveAbility::BindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	if (GeoLib::IsServer(GetWorld()))
	{
		HolderASC.OnDeployableLanded.AddUObject(this, &ThisClass::SplitDeployable);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSplitPassiveAbility::UnbindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	HolderASC.OnDeployableLanded.RemoveAll(this);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSplitPassiveAbility::SplitDeployable(AGeoDeployableBase& Deployable)
{
	if (!TryTrigger())
	{
		return;
	}
	float const Angle = FMath::FRandRange(0.f, UE_TWO_PI);
	FVector const Offset =
		FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * FMath::FRandRange(MinCopyDistance, MaxCopyDistance);
	GeoASLib::SpawnDeployableFromAbility(*GetGeoAbilitySystemComponentFromActorInfo(),
										 Deployable.GetActorLocation() + Offset, StartHealthRatio,
										 Deployable.GetData()->Params.DeployDistance);
}
