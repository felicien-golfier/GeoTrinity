// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"
#include "HUD/Interface/GeoChargeBeamGaugeWidgetInterface.h"

#include "GeoChargeBeamGaugeWidget.generated.h"


class UGeoGameplayAbility;
class UProgressBar;

/**
 * World-space widget for the Circle charge-beam ability.
 * Placed on the character's ChargeBeamGaugeComponent (left of character).
 * ChargeBar fills with the ability's charge ratio.
 * SweetSpotBar overlays the sweet-spot window in a distinct color.
 */
UCLASS()
class GEOTRINITYUI_API UGeoChargeBeamGaugeWidget
	: public UGeoUserWidget
	, public IGeoChargeBeamGaugeWidgetInterface
{
	GENERATED_BODY()

public:
	/** The charging ability that drives ChargeBar fill. Must be set before the widget ticks. */
	UPROPERTY(BlueprintReadOnly, Category = "ChargeBeam")
	TObjectPtr<UGeoGameplayAbility> ChargeBeamAbility;

	/** Sets the charging ability that drives ChargeBar fill. */
	virtual void SetChargeBeamAbility(UGeoGameplayAbility* Ability) override { ChargeBeamAbility = Ability; }
	/** Sets the sweet-spot window boundaries and marks the SweetSpotBar position dirty for update on next tick. */
	virtual void SetSweetSpotRatios(float MinRatio, float MaxRatio) override;
	/** Syncs ChargeBar and SweetSpotBar fill/color to the current ability charge ratio. Safe to call outside of tick. */
	virtual void UpdateVisualChargeRatio() const override;

protected:
	/** Applies any pending sweet-spot layout changes, then updates the charge bar fill each frame. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	/** Main charge fill bar. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ChargeBar;

	/** Overlay bar sized to the sweet-spot window, rendered in a distinct color. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> SweetSpotBar;

private:
	void UpdateSweetSpotLayout();

	float SweetSpotMinRatio = 0.6f;
	float SweetSpotMaxRatio = 0.7f;
	bool SweetSpotRatioDirty = false;
};
