// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoChargeBeamGaugeWidget.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamGaugeWidget::SetSweetSpotRatios(float MinRatio, float MaxRatio)
{
	SweetSpotMinRatio = MinRatio;
	SweetSpotMaxRatio = MaxRatio;
	SweetSpotRatioDirty = true;
}

void UGeoChargeBeamGaugeWidget::UpdateVisualChargeRatio() const
{
	float const ChargeRatio = ChargeBeamAbility->GetChargeRatio();

	if (ChargeBar)
	{
		ChargeBar->SetPercent(ChargeRatio);
	}

	if (SweetSpotBar)
	{
		float const SweetSpotRange = SweetSpotMaxRatio - SweetSpotMinRatio;

		if (ChargeRatio < SweetSpotMinRatio || SweetSpotRange <= 0.f)
		{
			FProgressBarStyle Style = SweetSpotBar->GetWidgetStyle();
			SweetSpotBar->SetWidgetStyle(Style);
			SweetSpotBar->SetPercent(0.f);
		}
		else
		{
			float const Fill = FMath::Clamp((ChargeRatio - SweetSpotMinRatio) / SweetSpotRange, 0.f, 1.f);
			SweetSpotBar->SetPercent(Fill);

			bool const bPastSweetSpot = ChargeRatio > SweetSpotMaxRatio;
			FLinearColor const FillColor =
				bPastSweetSpot ? FLinearColor(1.0f, 0.75f, 0.0f, .30f) : FLinearColor(1.0f, 0.75f, 0.0f, 1.0f);


			FProgressBarStyle Style = SweetSpotBar->GetWidgetStyle();
			Style.FillImage.TintColor = FSlateColor(FillColor);
			SweetSpotBar->SetWidgetStyle(Style);
		}
	}
}
// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamGaugeWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (SweetSpotRatioDirty)
	{
		UpdateSweetSpotLayout();
	}

	if (!ChargeBeamAbility)
	{
		return;
	}

	UpdateVisualChargeRatio();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamGaugeWidget::UpdateSweetSpotLayout()
{
	if (!SweetSpotBar)
	{
		return;
	}

	FVector2D const BarSize = GetCachedGeometry().GetLocalSize();
	if (BarSize.IsZero())
	{
		return;
	}

	UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(SweetSpotBar->Slot);
	if (!ensureMsgf(CanvasPanelSlot, TEXT("SweetSpotBar is not in a CanvasPanel")))
	{
		return;
	}

	float const SweetTop = (1.f - SweetSpotMaxRatio) * BarSize.Y;
	float const SweetHeight = (SweetSpotMaxRatio - SweetSpotMinRatio) * BarSize.Y;

	CanvasPanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
	CanvasPanelSlot->SetOffsets(FMargin(0.f, SweetTop, 0.f, SweetHeight));
	SweetSpotRatioDirty = false;
}
