// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/HudFunctionLibrary.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"

bool UHudFunctionLibrary::ShouldDrawHUD(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	// Don't draw on dedicated servers
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	return true; // Standalone, client, or listen server
}

float UHudFunctionLibrary::GetHealthRatio(const UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
	{
		return 0.0f;
	}
	const float CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
	const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return CurrentHealth / MaxHealth;
}