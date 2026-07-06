// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "HUD/GenericCombattantWidget.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"
#include "HUD/HudFunctionLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGenericCombattantWidget::InitializeWithAbilitySystemComponent_Implementation(UAbilitySystemComponent* ASC)
{
	// Idempotent: the owner may re-initialize once its ASC is ready. Bound to a different ASC → unbind before rebinding
	// so callbacks don't stack. Same ASC → fall through and refresh stats only (no rebind).
	if (OwnerASC.IsValid() && OwnerASC != ASC)
	{
		UnbindStatCallbacks();
	}

	bool const bAlreadyBound = (OwnerASC == ASC);
	OwnerASC = ASC;

	if (bAlreadyBound)
	{
		// Already bound to this ASC: just refresh the displayed values (attributes may have been 0 at first bind).
		InitStats();
		return;
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (AGeoHUD* GeoHUD = PlayerController->GetHUD<AGeoHUD>())
		{
			InitFromHUD(GeoHUD);
			GeoHUD->RegisterASCForDamageNumbers(ASC, ASC->GetAvatarActor());
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

	UGeoAbilitySystemComponent* GeoASC = Cast<UGeoAbilitySystemComponent>(OwnerASC.Get());
	if (!GeoASC)
	{
		return;
	}

	GeoASC->OnHealthChanged.AddDynamic(this, &UGenericCombattantWidget::OnHealthChanged);
	GeoASC->OnMaxHealthChanged.AddDynamic(this, &UGenericCombattantWidget::OnHealthChanged);

	// The ASC exposes no Shield delegate (unlike Health/MaxHealth), so bind the attribute directly. A weak lambda
	// self-cleans when this widget is destroyed, so no matching removal is needed in UnbindStatCallbacks.
	GeoASC->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetShieldAttribute())
		.AddWeakLambda(this,
					   [this](FOnAttributeChangeData const&)
					   {
						   RefreshShield();
					   });
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
	// MaxHealth is the shield denominator, so a max-health change moves the shield ratio too.
	RefreshShield();
	// Re-evaluate visibility too: on the listen-server host InitStats() runs before the owner's attributes are
	// initialized, reading MaxHealth as 0 and collapsing the bar. Clients recover because MaxHealth arrives via
	// replication and fires this delegate, but the host sets it synchronously — so without this call the bar stays
	// collapsed on the host forever.
	UpdateHealthBarVisibility();
}
