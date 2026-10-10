// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoWakePassiveAbility.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoWakePassiveAbility::UGeoWakePassiveAbility()
{
	CooldownSeconds = 8.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWakePassiveAbility::BindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	if (GeoLib::IsServer(GetWorld()))
	{
		HolderASC.OnDashStarted.AddUObject(this, &ThisClass::OnDashStarted);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWakePassiveAbility::UnbindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	HolderASC.OnDashStarted.RemoveAll(this);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWakePassiveAbility::OnDashStarted(FVector const& DashStart)
{
	if (TryTrigger())
	{
		UGeoDeployAbility::SpawnDeployableAt(*GetGeoAbilitySystemComponentFromActorInfo(), DashStart);
	}
}
