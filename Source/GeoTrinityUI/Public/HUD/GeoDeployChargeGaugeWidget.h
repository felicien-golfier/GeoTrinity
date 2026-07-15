// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"
#include "HUD/Interface/GeoDeployGaugeWidgetInterface.h"

#include "GeoDeployChargeGaugeWidget.generated.h"


class UGeoGameplayAbility;
class UProgressBar;

/**
 * World-space widget that shows the charge fill of a deploy ability as a progress bar.
 * Placed on the character's WidgetComponent. Set DeployAbility before adding to viewport.
 */
UCLASS()
class GEOTRINITYUI_API UGeoDeployChargeGaugeWidget
	: public UGeoUserWidget
	, public IGeoDeployGaugeWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sets the deploy ability whose charge ratio drives the ChargeBar fill. */
	virtual void SetDeployAbility(UGeoGameplayAbility* Ability) override { DeployAbility = Ability; }

	/** Syncs ChargeBar fill to the current DeployAbility charge ratio. Safe to call outside of tick. */
	virtual void UpdateVisualChargeRatio() const override;

	/** The deploy ability whose charge ratio drives the ChargeBar fill. Must be set before the widget ticks. */
	UPROPERTY(BlueprintReadOnly, Category = "Deploy")
	TObjectPtr<UGeoGameplayAbility> DeployAbility;

protected:
	/** Updates ChargeBar percent from DeployAbility->GetChargeRatio() each frame. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ChargeBar;
};
