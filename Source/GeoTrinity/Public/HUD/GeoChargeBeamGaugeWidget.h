// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

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
class GEOTRINITY_API UGeoChargeBeamGaugeWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** The charging ability that drives ChargeBar fill. Must be set before the widget ticks. */
	UPROPERTY(BlueprintReadOnly, Category = "ChargeBeam")
	TObjectPtr<UGeoGameplayAbility> ChargeBeamAbility;

	void SetSweetSpotRatios(float MinRatio, float MaxRatio);
	void UpdateVisualChargeRatio() const;

protected:
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
