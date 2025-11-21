// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GeoAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoAttributeSetBase, Health, OldHealth);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoAttributeSetBase, MaxHealth, OldMaxHealth);
}
