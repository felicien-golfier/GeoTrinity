// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/GenericCombattantWidget.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"
#include "HUD/HudFunctionLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::InitializeWithAbilitySystemComponent_Implementation(UAbilitySystemComponent* ASC)
{
	OwnerASC = ASC;

	if (!GetOwningPlayer())
	{
		UE_LOG(LogGeoTrinity, Log, TEXT("No owning player for widget %s"), *GetName());
		return;
	}

	if (AGeoHUD* Hud = Cast<AGeoHUD>(GetOwningPlayer()->GetHUD()))
	{
		InitFromHUD(Hud);
	}

	BindStatCallbacks();
	InitStats();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::InitStats()
{
	if (OwnerASC.IsValid())
	{
		UpdateHealthRatio(UHudFunctionLibrary::GetHealthRatio(OwnerASC.Get()));
	}
	else
	{
		UE_LOG(LogGeoTrinity, Warning,
			   TEXT("Initializing UI stats with default values, probably not ideal. Please fix missing OwnerASC in %s"),
			   *GetName());
		UpdateHealthRatio(1.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::BindStatCallbacks()
{
	if (!OwnerASC.IsValid())
	{
		return;
	}

	UGeoAbilitySystemComponent* GeoASC = Cast<UGeoAbilitySystemComponent>(OwnerASC.Get());
	if (!GeoASC)
	{
		return;
	}

	GeoASC->OnHealthChanged.AddDynamic(this, &UGenericCombattantWidget::OnHealthChanged);
	GeoASC->OnMaxHealthChanged.AddDynamic(this, &UGenericCombattantWidget::OnHealthChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::OnHealthChanged(float NewValue)
{
	// Don't use NewValue, just get the ratio from ASC (it might be health or max health)
	UpdateHealthRatio(UHudFunctionLibrary::GetHealthRatio(OwnerASC.Get()));
}
