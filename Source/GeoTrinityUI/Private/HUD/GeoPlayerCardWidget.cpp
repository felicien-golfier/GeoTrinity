// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoPlayerCardWidget.h"

#include "AbilitySystem/Abilities/Circle/GeoSweetSpotChargePassiveAbility.h"
#include "AbilitySystem/Abilities/Square/GeoSacrificeDetonateAbility.h"
#include "AbilitySystem/Abilities/Triangle/GeoRecallTurretAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/TextBlock.h"
#include "GameClasses/GeoPlayerState.h"
#include "HUD/Style/GeoMeter.h"
#include "HUD/Style/GeoShape.h"
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
	ESlateVisibility const GaugeVisibility =
		GetClassGauge(Fill, bReady) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

	if (GaugeRing)
	{
		GaugeRing->SetVisibility(GaugeVisibility);
		GaugeRing->SetFill(Fill);
		GaugeRing->SetReady(bReady);
	}
	if (GaugeText)
	{
		GaugeText->SetVisibility(GaugeVisibility);
		GaugeText->SetText(FText::Format(GaugeFormat, FText::AsNumber(FMath::RoundToInt(Fill * 100.f))));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoPlayerCardWidget::GetClassGauge(float& OutFill, bool& bOutReady) const
{
	OutFill = 0.f;
	bOutReady = false;
	UAbilitySystemComponent const* ASC = OwnerASC.Get();
	if (!ASC)
	{
		return false;
	}

	UGeoSweetSpotChargePassiveAbility const* SweetSpot =
		GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(*ASC);
	UGeoSacrificeDetonateAbility const* Detonate = GeoASLib::GetGrantedAbility<UGeoSacrificeDetonateAbility>(*ASC);
	UGeoRecallTurretAbility const* Recall = GeoASLib::GetGrantedAbility<UGeoRecallTurretAbility>(*ASC);
	if (SweetSpot)
	{
		OutFill = SweetSpot->GetGaugeRatio(*ASC);
		bOutReady = OutFill >= 1.f;
	}
	else if (Detonate)
	{
		float const Sacrificed = ASC->GetNumericAttribute(UCharacterAttributeSet::GetSacrificeValueAttribute());
		OutFill = FMath::Clamp(Sacrificed / SacrificeForFullGauge, 0.f, 1.f);
		bOutReady = OutFill >= 1.f;
	}
	else if (Recall)
	{
		bOutReady = Recall->IsBlinkRecallReady(*ASC);
		OutFill = bOutReady ? 1.f : 0.f;
	}
	return SweetSpot || Detonate || Recall;
}
