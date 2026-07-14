// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoStatusBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HUD/GeoHUD.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Styling/CoreStyle.h"

namespace
{
	FName const DepletionFillParam(TEXT("Fill"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatusBarWidget::InitStatusBar(AGeoHUD* GeoHUD)
{
	HUD = GeoHUD;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoStatusBarWidget::Initialize()
{
	bool const bResult = Super::Initialize();

	if (WidgetTree && !StatusBox)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		WidgetTree->RootWidget = Canvas;

		// Wrapper fills the whole bar; the box inside is centered and only as wide as its icons.
		UOverlay* Wrapper = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Wrapper);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));

		StatusBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusBox"));
		UOverlaySlot* BoxSlot = Wrapper->AddChildToOverlay(StatusBox);
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	return bResult;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatusBarWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HUD || !StatusBox)
	{
		return;
	}

	TArray<FGeoActiveEffectIcon> const Entries = HUD->GetActiveEffectIcons();

	TArray<UObject*> Icons;
	for (FGeoActiveEffectIcon const& Entry : Entries)
	{
		Icons.Add(Entry.Icon);
	}

	if (Icons != DisplayedIcons)
	{
		DisplayedIcons = Icons;
		StatusBox->ClearChildren();
		IconImages.Reset();
		CountTexts.Reset();
		TimerTexts.Reset();
		DepletionSweeps.Reset();
		DepletionSweepMIDs.Reset();
		AppliedIconSize = 0.f;

		for (FGeoActiveEffectIcon const& Entry : Entries)
		{
			UOverlay* EntryOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

			UImage* IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (UTexture2D* Texture = Cast<UTexture2D>(Entry.Icon))
			{
				IconImage->SetBrushFromTexture(Texture);
			}
			else if (UMaterialInterface* Material = Cast<UMaterialInterface>(Entry.Icon))
			{
				IconImage->SetBrushFromMaterial(Material);
			}
			// Round icon: half-height rounding turns the square brush into a circle.
			FSlateBrush RoundBrush = IconImage->GetBrush();
			RoundBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			RoundBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			IconImage->SetBrush(RoundBrush);
			EntryOverlay->AddChildToOverlay(IconImage);
			IconImages.Add(IconImage);

			UImage* SweepImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			UMaterialInstanceDynamic* SweepMID = nullptr;
			if (DepletionSweepMaterial)
			{
				SweepMID = UMaterialInstanceDynamic::Create(DepletionSweepMaterial, this);
				SweepMID->SetScalarParameterValue(DepletionFillParam, 0.f);
				SweepImage->SetBrushFromMaterial(SweepMID);
			}
			else
			{
				SweepImage->SetVisibility(ESlateVisibility::Collapsed);
			}
			EntryOverlay->AddChildToOverlay(SweepImage);
			DepletionSweeps.Add(SweepImage);
			DepletionSweepMIDs.Add(SweepMID);

			UTextBlock* TimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			TimerText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
			UOverlaySlot* TimerSlot = EntryOverlay->AddChildToOverlay(TimerText);
			TimerSlot->SetHorizontalAlignment(HAlign_Center);
			TimerSlot->SetVerticalAlignment(VAlign_Center);
			TimerTexts.Add(TimerText);

			UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			CountText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
			UOverlaySlot* CountSlot = EntryOverlay->AddChildToOverlay(CountText);
			CountSlot->SetHorizontalAlignment(HAlign_Right);
			CountSlot->SetVerticalAlignment(VAlign_Bottom);
			CountTexts.Add(CountText);

			StatusBox->AddChildToHorizontalBox(EntryOverlay)->SetPadding(FMargin(2.f, 0.f));
		}
	}

	float const IconSize = MyGeometry.GetLocalSize().Y;
	if (!FMath::IsNearlyEqual(IconSize, AppliedIconSize))
	{
		AppliedIconSize = IconSize;
		for (int32 Index = 0; Index < IconImages.Num(); ++Index)
		{
			IconImages[Index]->SetDesiredSizeOverride(FVector2D(IconSize));
			DepletionSweeps[Index]->SetDesiredSizeOverride(FVector2D(IconSize));
		}
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		FGeoActiveEffectIcon const& Entry = Entries[Index];

		TimerTexts[Index]->SetVisibility(Entry.TimeRemaining < 0.f ? ESlateVisibility::Hidden
																   : ESlateVisibility::HitTestInvisible);
		if (Entry.TimeRemaining >= 0.f)
		{
			TimerTexts[Index]->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Entry.TimeRemaining)));
		}

		CountTexts[Index]->SetVisibility(Entry.Count > 1 ? ESlateVisibility::HitTestInvisible
														 : ESlateVisibility::Hidden);
		if (Entry.Count > 1)
		{
			CountTexts[Index]->SetText(FText::AsNumber(Entry.Count));
		}

		if (DepletionSweepMIDs[Index])
		{
			float const Fill = (Entry.TimeRemaining < 0.f || Entry.Duration <= 0.f)
									? 0.f
									: 1.f - FMath::Clamp(Entry.TimeRemaining / Entry.Duration, 0.f, 1.f);
			DepletionSweepMIDs[Index]->SetScalarParameterValue(DepletionFillParam, Fill);
		}
	}
}
