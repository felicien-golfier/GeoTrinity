// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"

#include "Net/UnrealNetwork.h"

UGeoGemAttributeSet::UGeoGemAttributeSet()
{
	InitCritChance(0.1f);
	InitCritDamage(1.5f);
	InitDashDistanceMultiplier(1.f);
	InitDeployableBlinkMultiplier(1.f);
	InitDeployableHealthMultiplier(1.f);
	InitDeployableDrainMultiplier(1.f);
}

void UGeoGemAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGeoGemAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoGemAttributeSet, CritDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoGemAttributeSet, DashDistanceMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoGemAttributeSet, DeployableBlinkMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoGemAttributeSet, DeployableHealthMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoGemAttributeSet, DeployableDrainMultiplier, COND_None, REPNOTIFY_Always);
}

void UGeoGemAttributeSet::OnRep_CritChance(FGameplayAttributeData const& OldCritChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoGemAttributeSet, CritChance, OldCritChance);
}

void UGeoGemAttributeSet::OnRep_CritDamage(FGameplayAttributeData const& OldCritDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoGemAttributeSet, CritDamage, OldCritDamage);
}

void UGeoGemAttributeSet::OnRep_DashDistanceMultiplier(FGameplayAttributeData const& OldDashDistanceMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoGemAttributeSet, DashDistanceMultiplier, OldDashDistanceMultiplier);
}

void UGeoGemAttributeSet::OnRep_DeployableBlinkMultiplier(FGameplayAttributeData const& OldDeployableBlinkMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoGemAttributeSet, DeployableBlinkMultiplier, OldDeployableBlinkMultiplier);
}

void UGeoGemAttributeSet::OnRep_DeployableHealthMultiplier(FGameplayAttributeData const& OldDeployableHealthMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoGemAttributeSet, DeployableHealthMultiplier, OldDeployableHealthMultiplier);
}

void UGeoGemAttributeSet::OnRep_DeployableDrainMultiplier(FGameplayAttributeData const& OldDeployableDrainMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoGemAttributeSet, DeployableDrainMultiplier, OldDeployableDrainMultiplier);
}
