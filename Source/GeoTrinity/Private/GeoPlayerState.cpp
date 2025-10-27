// Fill out your copyright notice in the Description page of Project Settings.


#include "GeoPlayerState.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"

AGeoPlayerState::AGeoPlayerState()
{
	// Create ASC, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// If replication is not sufficient in this mode, adapt. (From what I remember, the replication can anyway be tailored
	// in the Abilities/Gameplay effects)
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	SetNetUpdateFrequency(100.f);
}

UAbilitySystemComponent* AGeoPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}