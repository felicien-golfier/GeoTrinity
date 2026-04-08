// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "HUD/GenericCombattantWidget.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"
#include "HUD/HudFunctionLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::InitializeWithAbilitySystemComponent_Implementation(UAbilitySystemComponent* ASC)
{
	OwnerASC = ASC;

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (AGeoHUD* Hud = Cast<AGeoHUD>(PlayerController->GetHUD()))
		{
			InitFromHUD(Hud);
		}
	}

	BindStatCallbacks();
	InitStats();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::UpdateHealthRatio_Implementation(float NewHealthRatio)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(NewHealthRatio);
		FLinearColor const HighHealth = FLinearColor::LerpUsingHSV(
			FLinearColor::Yellow, FLinearColor::Green, FMath::Clamp((NewHealthRatio - 0.5f) * 2.f, 0.f, 1.f));
		FLinearColor const LowHealth = FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Yellow,
																  FMath::Clamp(NewHealthRatio * 2.f, 0.f, 1.f));
		HealthBar->SetFillColorAndOpacity(NewHealthRatio > 0.5f ? HighHealth : LowHealth);
	}
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
	UpdateHealthBarVisibility();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::UpdateHealthBarVisibility_Implementation()
{
	if (!HealthBar || !OwnerASC.IsValid())
	{
		return;
	}

	float const MaxHealth = OwnerASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	HealthBar->SetVisibility(MaxHealth > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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
void UGenericCombattantWidget::UnbindStatCallbacks()
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
	GeoASC->OnHealthChanged.RemoveDynamic(this, &UGenericCombattantWidget::OnHealthChanged);
	GeoASC->OnMaxHealthChanged.RemoveDynamic(this, &UGenericCombattantWidget::OnHealthChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::OnHealthChanged(float NewValue)
{
	if (!OwnerASC.IsValid())
	{
		return;
	}
	// Don't use NewValue, just get the ratio from ASC (it might be health or max health)
	UpdateHealthRatio(UHudFunctionLibrary::GetHealthRatio(OwnerASC.Get()));
}
