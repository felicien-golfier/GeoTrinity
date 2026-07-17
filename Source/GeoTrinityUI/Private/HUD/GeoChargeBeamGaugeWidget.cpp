// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoChargeBeamGaugeWidget.h"

#include "AbilitySystem/Abilities/Circle/GeoSweetSpotChargePassiveAbility.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

namespace
{
	int32 constexpr SweetSpotGradientBandCount = 7;
	constexpr FLinearColor SweetSpotGradientColor(1.0f, 0.75f, 0.0f);
	float constexpr SweetSpotGradientCenterAlpha = 0.85f;
	float constexpr SweetSpotGradientEdgeAlpha = 0.1f;
} // namespace

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

	UAbilitySystemComponent const* ASC = ChargeBeamAbility->GetAbilitySystemComponentFromActorInfo();
	UGeoSweetSpotChargePassiveAbility const* Passive =
		ASC ? GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(*ASC) : nullptr;
	bool const bGaugeFull = Passive && Passive->GetGaugeRatio(*ASC) >= 1.f;
	for (UImage* const Band : SweetSpotGradientBands)
	{
		Band->SetVisibility(bGaugeFull ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
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

	if (SweetSpotGradientBands.IsEmpty())
	{
		UCanvasPanel* CanvasPanel = CastChecked<UCanvasPanel>(SweetSpotBar->GetParent());
		for (int32 Index = 0; Index < SweetSpotGradientBandCount; ++Index)
		{
			UImage* Band = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			float const DistanceToCenter = FMath::Abs((Index + 0.5f) / SweetSpotGradientBandCount - 0.5f) * 2.f;
			FLinearColor BandColor = SweetSpotGradientColor;
			BandColor.A = FMath::Lerp(SweetSpotGradientCenterAlpha, SweetSpotGradientEdgeAlpha, DistanceToCenter);
			Band->SetColorAndOpacity(BandColor);
			Band->SetVisibility(ESlateVisibility::Collapsed);
			CanvasPanel->AddChildToCanvas(Band);
			SweetSpotGradientBands.Add(Band);
		}
	}
	float const BandHeight = SweetHeight / SweetSpotGradientBandCount;
	for (int32 Index = 0; Index < SweetSpotGradientBands.Num(); ++Index)
	{
		UCanvasPanelSlot* BandSlot = CastChecked<UCanvasPanelSlot>(SweetSpotGradientBands[Index]->Slot);
		BandSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		BandSlot->SetOffsets(FMargin(0.f, SweetTop + Index * BandHeight, 0.f, BandHeight));
	}

	SweetSpotRatioDirty = false;
}
