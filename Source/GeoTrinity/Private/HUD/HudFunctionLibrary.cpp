// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/HudFunctionLibrary.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"

bool UHudFunctionLibrary::ShouldDrawHUD(UObject const* WorldContextObject)
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

float UHudFunctionLibrary::GetHealthRatio(UAbilitySystemComponent const* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
	{
		return 0.0f;
	}
	float const CurrentHealth = AbilitySystemComponent->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
	float const MaxHealth = AbilitySystemComponent->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return CurrentHealth / MaxHealth;
}
