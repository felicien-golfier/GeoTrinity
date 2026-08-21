// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "HUD/GenericCombattantWidget.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"
#include "HUD/HudFunctionLibrary.h"

// Tint the whole bar drops to once its owner has no life left.
static FLinearColor const DeadTint{0.35f, 0.35f, 0.35f, 1.f};

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::InitializeWithAbilitySystemComponent_Implementation(UAbilitySystemComponent* ASC)
{
	// Idempotent: the owner may re-initialize once its ASC is ready. Same ASC → refresh the stats only (no rebind).
	bool const bAlreadyBound = (OwnerASC == ASC);
	OwnerASC = ASC;

	AGeoHUD* GeoHUD = nullptr;
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		GeoHUD = PlayerController->GetHUD<AGeoHUD>();
	}

	if (bAlreadyBound)
	{
		// Already bound to this ASC: just refresh the displayed values (attributes may have been 0 at first bind).
		// Retry damage-number registration too — the avatar may not have existed at the first bind.
		if (GeoHUD)
		{
			GeoHUD->RegisterASCForDamageNumbers(ASC, ASC->GetAvatarActor());
		}
		RefreshStats();
		return;
	}

	if (GeoHUD)
	{
		InitFromHUD(GeoHUD);
		GeoHUD->RegisterASCForDamageNumbers(ASC, ASC->GetAvatarActor());
	}

	BindStatCallbacks();
	RefreshStats();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::UpdateHealthRatio_Implementation(float NewHealthRatio)
{
	// Death zeroes health and shield, so an empty bar means a downed combatant: grey the whole widget out.
	SetColorAndOpacity(NewHealthRatio > 0.f ? FLinearColor::White : DeadTint);

	if (HealthBar)
	{
		HealthBar->SetPercent(NewHealthRatio);
		FLinearColor const HighHealth = FLinearColor::LerpUsingHSV(
			FLinearColor::Yellow, FLinearColor::Green, FMath::Clamp((NewHealthRatio - 0.5f) * 2.f, 0.f, 1.f));
		FLinearColor const LowHealth = FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Yellow,
																  FMath::Clamp(NewHealthRatio * 2.f, 0.f, 1.f));
		HealthBar->SetFillColorAndOpacity(NewHealthRatio > 0.5f ? HighHealth : LowHealth);
	}

	if (CurrentHealthText && OwnerASC.IsValid())
	{
		float const Health = OwnerASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
		CurrentHealthText->SetText(FText::AsNumber(FMath::RoundToInt(Health)));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::UpdateShieldRatio_Implementation(float NewShieldRatio)
{
	if (ShieldBar)
	{
		ShieldBar->SetPercent(NewShieldRatio);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::RefreshShield()
{
	if (!OwnerASC.IsValid())
	{
		return;
	}
	float const MaxHealth = OwnerASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	float const Shield = OwnerASC->GetNumericAttribute(UGeoAttributeSetBase::GetShieldAttribute());
	UpdateShieldRatio(MaxHealth > 0.f ? Shield / MaxHealth : 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::RefreshStats()
{
	if (OwnerASC.IsValid())
	{
		UpdateHealthRatio(UHudFunctionLibrary::GetHealthRatio(OwnerASC.Get()));
	}
	else
	{
		UE_LOG(LogGeoTrinity, Warning,
			   TEXT("Showing UI stats with default values, probably not ideal. Please fix missing OwnerASC in %s"),
			   *GetName());
		UpdateHealthRatio(1.f);
	}
	RefreshShield();
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

	// A weak lambda self-cleans when this widget is destroyed, so there is no matching removal. Re-binding to another
	// ASC leaves the old bindings behind, which is harmless: RefreshStats always reads the current OwnerASC.
	for (FGameplayAttribute const& Attribute :
		 {UGeoAttributeSetBase::GetHealthAttribute(), UGeoAttributeSetBase::GetMaxHealthAttribute(),
		  UGeoAttributeSetBase::GetShieldAttribute()})
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this,
																				  [this](FOnAttributeChangeData const&)
																				  {
																					  RefreshStats();
																				  });
	}
}
