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
#include "Materials/MaterialInterface.h"
#include "Styling/CoreStyle.h"

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

		StatusBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusBox"));
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(StatusBox);
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.f));
		CanvasSlot->SetAutoSize(true);
		// Bottom-center, above the ability bar.
		CanvasSlot->SetPosition(FVector2D(0.f, -140.f));
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
		CountTexts.Reset();
		TimerTexts.Reset();

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
			IconImage->SetDesiredSizeOverride(FVector2D(48.f));

			// Round icon: half-height rounding turns the square brush into a circle.
			FSlateBrush RoundBrush = IconImage->GetBrush();
			RoundBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			RoundBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			IconImage->SetBrush(RoundBrush);
			EntryOverlay->AddChildToOverlay(IconImage);

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
	}
}
