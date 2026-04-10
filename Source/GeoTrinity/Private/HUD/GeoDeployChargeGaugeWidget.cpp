// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoDeployChargeGaugeWidget.h"

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "Components/ProgressBar.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployChargeGaugeWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ChargeBar || !DeployAbility)
	{
		ensureMsgf(false, TEXT("ChargeBar or DeployAbility is null"));
		return;
	}

	float const ChargeRatio = DeployAbility->GetChargeRatio();
	ChargeBar->SetPercent(ChargeRatio);
}
