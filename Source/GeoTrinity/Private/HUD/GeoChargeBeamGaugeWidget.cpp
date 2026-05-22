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
	if (!UpdateSweetSpotLayout())
	{
		bPendingSweetSpotLayout = true;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamGaugeWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bPendingSweetSpotLayout)
	{
		bPendingSweetSpotLayout = !UpdateSweetSpotLayout();
	}

	if (!ChargeBeamAbility)
	{
		return;
	}

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
			SweetSpotBar->SetPercent(0.f);
		}
		else
		{
			float const Fill = FMath::Clamp((ChargeRatio - SweetSpotMinRatio) / SweetSpotRange, 0.f, 1.f);
			SweetSpotBar->SetPercent(Fill);

			bool const bPastSweetSpot = ChargeRatio > SweetSpotMaxRatio;
			FLinearColor const FillColor =
				bPastSweetSpot ? FLinearColor(1.0f, 0.3f, 0.0f, 1.0f) : FLinearColor(1.0f, 0.75f, 0.0f, 1.0f);

			FProgressBarStyle Style = SweetSpotBar->GetWidgetStyle();
			Style.FillImage.TintColor = FSlateColor(FillColor);
			SweetSpotBar->SetWidgetStyle(Style);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoChargeBeamGaugeWidget::UpdateSweetSpotLayout()
{
	if (!SweetSpotBar)
	{
		return true;
	}

	FVector2D const WidgetSize = GetCachedGeometry().GetLocalSize();
	if (WidgetSize.IsZero())
	{
		return false;
	}

	UCanvasPanelSlot* CanvasPanelSlot = Cast<UCanvasPanelSlot>(SweetSpotBar->Slot);
	if (!ensureMsgf(CanvasPanelSlot, TEXT("SweetSpotBar is not in a CanvasPanel")))
	{
		return true;
	}

	float const SweetTop = (1.f - SweetSpotMaxRatio) * WidgetSize.Y;
	float const SweetHeight = (SweetSpotMaxRatio - SweetSpotMinRatio) * WidgetSize.Y;

	CanvasPanelSlot->SetOffsets(FMargin(0.f, SweetTop, WidgetSize.X, SweetHeight));
	return true;
}
