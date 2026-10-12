// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Team/GeoPlayerCardWidget.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"
#include "GameClasses/Match/GeoPlayerState.h"
#include "HUD/Style/Meter/GeoMeter.h"
#include "HUD/Style/Glyph/GeoShape.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPlayerCardWidget::InitForPlayer(AGeoPlayerState* InPlayerState)
{
	if (!ensureMsgf(InPlayerState, TEXT("%hs: no player state on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}

	PlayerState = InPlayerState;
	if (NameText)
	{
		NameText->SetText(FText::FromString(InPlayerState->GetPlayerName()));
	}
	InitializeWithAbilitySystemComponent(InPlayerState->GetAbilitySystemComponent());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPlayerCardWidget::NativeTick(FGeometry const& MyGeometry, float const InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (AGeoPlayerState const* ShownPlayer = PlayerState.Get())
	{
		if (ShownClass != ShownPlayer->GetPlayerClass())
		{
			ShowClass(ShownPlayer->GetPlayerClass());
		}
		RefreshGauge();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPlayerCardWidget::RefreshStats()
{
	Super::RefreshStats();

	if (HealthText && OwnerASC.IsValid())
	{
		float const Health = OwnerASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
		float const MaxHealth = OwnerASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
		HealthText->SetText(FText::Format(HealthFormat, FText::AsNumber(FMath::RoundToInt(Health)),
										  FText::AsNumber(FMath::RoundToInt(MaxHealth))));
	}

	if (ShieldText && OwnerASC.IsValid())
	{
		int32 const Shield = FMath::RoundToInt(OwnerASC->GetNumericAttribute(UGeoAttributeSetBase::GetShieldAttribute()));
		ShieldText->SetVisibility(Shield > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		ShieldText->SetText(FText::Format(ShieldFormat, FText::AsNumber(Shield)));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPlayerCardWidget::ShowClass(EPlayerClass const PlayerClass)
{
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(PlayerClass) : nullptr;
	ShownClass = PlayerClass;
	if (ClassStyle && ClassShape)
	{
		ClassShape->SetClassShape(*ClassStyle);
	}
	if (ClassStyle && GaugeRing)
	{
		GaugeRing->SetRingShape(ClassStyle->Sides, ClassStyle->Rotation);
	}
	if (ClassStyle && RoleText)
	{
		RoleText->SetText(ClassStyle->Role);
	}
	for (UGeoMeter* Meter : {HealthMeter.Get(), GaugeRing.Get()})
	{
		if (ClassStyle && Meter)
		{
			Meter->SetFillTint(ClassStyle->Color);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoPlayerCardWidget::RefreshGauge()
{
	float Fill = 0.f;
	bool bReady = false;
	FText Label;
	bool const bHasGauge = GetClassGauge(Fill, bReady, Label);

	if (GaugeRing)
	{
		GaugeRing->SetVisibility(bHasGauge ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		GaugeRing->SetFill(Fill);
		GaugeRing->SetReady(bReady);
	}
	if (GaugeText)
	{
		GaugeText->SetVisibility(Label.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		GaugeText->SetText(FText::Format(GaugeFormat, Label));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoPlayerCardWidget::GetClassGauge(float& OutFill, bool& bOutReady, FText& OutLabel) const
{
	OutFill = 0.f;
	bOutReady = false;
	OutLabel = FText::GetEmpty();
	UAbilitySystemComponent const* ASC = OwnerASC.Get();
	if (!ASC)
	{
		return false;
	}

	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		UGeoGameplayAbility const* Ability = Cast<UGeoGameplayAbility>(Spec.Ability);
		if (Ability && Ability->GetClassGauge(*ASC, OutFill, bOutReady))
		{
			Ability->GetGaugeLabel(*ASC, OutLabel);
			return true;
		}
	}

	return false;
}
