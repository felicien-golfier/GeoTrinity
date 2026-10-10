// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoSplitPassiveAbility.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoSplitPassiveAbility::UGeoSplitPassiveAbility()
{
	CooldownSeconds = 10.f;
}

// ---------------------------------------------------------------------------------------------------------------------
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
	if (TryTrigger())
	{
		FDeployableData const& Data = *Deployable.GetData();
		FAbilityPayload Payload;
		Payload.SourceOwner = Data.Owner;
		Payload.SourceAvatar = Data.Instigator;
		Payload.AbilityLevel = Data.Level;
		Payload.AbilityTag = Data.AbilityTag;
		Payload.Seed = Data.Seed;

		FDeployableDataParams CopyParams = Data.Params;
		CopyParams.HealthMultiplier *= CopyHealthFraction;

		float const Angle = FMath::FRandRange(0.f, UE_TWO_PI);
		FVector const Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f)
			* FMath::FRandRange(MinCopyDistance, MaxCopyDistance);
		GeoASLib::FullySpawnDeployable(Deployable.GetClass(), Payload, Data.EffectDataArray, CopyParams,
									   FTransform(Deployable.GetActorLocation() + Offset));
	}
}
