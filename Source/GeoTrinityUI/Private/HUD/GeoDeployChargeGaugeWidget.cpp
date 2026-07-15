// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoDeployChargeGaugeWidget.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Components/ProgressBar.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployChargeGaugeWidget::UpdateVisualChargeRatio() const
{
	if (!ChargeBar)
	{
		ensureMsgf(false, TEXT("ChargeBar is null"));
		return;
	}

	ChargeBar->SetPercent(DeployAbility->GetChargeRatio());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployChargeGaugeWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!DeployAbility)
	{
		return;
	}

	UpdateVisualChargeRatio();
}
