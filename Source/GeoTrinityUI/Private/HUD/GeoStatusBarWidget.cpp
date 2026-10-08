// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoStatusBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "HUD/GeoHUD.h"
#include "HUD/Style/GeoFrame.h"
#include "HUD/Style/GeoIconImage.h"
#include "HUD/Style/GeoMeter.h"
#include "HUD/Style/GeoUITheme.h"
#include "Tool/GeoIcon.h"

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
		StatusBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusBox"));
		WidgetTree->RootWidget = StatusBox;
	}

	return bResult;
}

// ---------------------------------------------------------------------------------------------------------------------
UTextBlock* UGeoStatusBarWidget::MakeText() const
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UGeoUITheme::ApplyTextStyle(Text, EGeoTextRole::Mono);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = TextSize;
	Text->SetFont(Font);
	return Text;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatusBarWidget::AddTile(FGeoActiveEffectIcon const& Entry)
{
	FGeoStatusTile& Tile = Tiles.AddDefaulted_GetRef();
	Tile.Icon = Entry.Icon;
	Tile.bDebuff = Entry.bDebuff;

	UOverlay* Content = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	UVerticalBox* Center = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UOverlaySlot* CenterSlot = Content->AddChildToOverlay(Center);
	CenterSlot->SetHorizontalAlignment(HAlign_Center);
	CenterSlot->SetVerticalAlignment(VAlign_Center);

	UGeoIconImage* IconImage = WidgetTree->ConstructWidget<UGeoIconImage>(UGeoIconImage::StaticClass());
	IconImage->SetIcon(Entry.Icon);
	IconImage->SetSize(IconSize);
	Center->AddChildToVerticalBox(IconImage)->SetHorizontalAlignment(HAlign_Center);

	Tile.ValueText = MakeText();
	Tile.ValueText->SetColorAndOpacity(Entry.Icon->Color.GetColor());
	Center->AddChildToVerticalBox(Tile.ValueText)->SetHorizontalAlignment(HAlign_Center);

	Tile.CountText = MakeText();
	UOverlaySlot* CountSlot = Content->AddChildToOverlay(Tile.CountText);
	CountSlot->SetHorizontalAlignment(HAlign_Right);
	CountSlot->SetVerticalAlignment(VAlign_Top);
	CountSlot->SetPadding(CountPadding);

	Tile.TimeMeter = WidgetTree->ConstructWidget<UGeoMeter>(UGeoMeter::StaticClass());
	Tile.TimeMeter->SetMeterStyle(TimeMeterStyle);
	Tile.TimeMeter->SetFillTint(Entry.bDebuff ? DebuffColor : FLinearColor::White);
	UOverlaySlot* MeterSlot = Content->AddChildToOverlay(Tile.TimeMeter);
	MeterSlot->SetHorizontalAlignment(HAlign_Fill);
	MeterSlot->SetVerticalAlignment(VAlign_Bottom);

	UGeoFrame* Frame = WidgetTree->ConstructWidget<UGeoFrame>(UGeoFrame::StaticClass());
	Frame->SetFrameStyle(Entry.bDebuff ? DebuffTileStyle : TileStyle);
	Frame->SetPadding(FMargin(0.f));
	Frame->SetContent(Content);

	USizeBox* TileBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	TileBox->SetWidthOverride(TileSize);
	TileBox->SetHeightOverride(TileSize);
	TileBox->SetContent(Frame);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(TileBox);

	Tile.TimeText = MakeText();
	if (Entry.bDebuff)
	{
		Tile.TimeText->SetColorAndOpacity(DebuffColor);
	}
	UVerticalBoxSlot* TimeSlot = Column->AddChildToVerticalBox(Tile.TimeText);
	TimeSlot->SetHorizontalAlignment(HAlign_Center);
	TimeSlot->SetPadding(FMargin(0.f, TimeGap, 0.f, 0.f));

	if ((Tiles.Num() - 1) % TilesPerRow == 0)
	{
		UPanelSlot* const RowSlot =
			StatusBox->InsertChildAt(0, WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass()));
		CastChecked<UVerticalBoxSlot>(RowSlot)->SetPadding(FMargin(0.f, TileGap, 0.f, 0.f));
	}
	CastChecked<UHorizontalBox>(StatusBox->GetChildAt(0))
		->AddChildToHorizontalBox(Column)
		->SetPadding(FMargin(0.f, 0.f, TileGap, 0.f));
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

	bool bSetChanged = Entries.Num() != Tiles.Num();
	for (int32 Index = 0; Index < Entries.Num() && !bSetChanged; ++Index)
	{
		bSetChanged = Entries[Index].Icon != Tiles[Index].Icon || Entries[Index].bDebuff != Tiles[Index].bDebuff;
	}

	if (bSetChanged)
	{
		StatusBox->ClearChildren();
		Tiles.Reset();
		for (FGeoActiveEffectIcon const& Entry : Entries)
		{
			AddTile(Entry);
		}
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		FGeoActiveEffectIcon const& Entry = Entries[Index];
		FGeoStatusTile const& Tile = Tiles[Index];

		FText const SecondsLeft =
			FText::FromString(FString::Printf(TEXT("%.0fs"), FMath::CeilToFloat(Entry.TimeRemaining)));
		Tile.TimeText->SetText(Entry.TimeRemaining < 0.f ? InfiniteTimeText : SecondsLeft);

		Tile.CountText->SetVisibility(Entry.Count > 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		Tile.CountText->SetText(FText::Format(CountFormat, Entry.Count));

		float const BoostPercent = FMath::RoundToFloat(Entry.BoostBonus * 100.f);
		Tile.ValueText->SetVisibility(BoostPercent == 0.f ? ESlateVisibility::Collapsed
														  : ESlateVisibility::HitTestInvisible);
		Tile.ValueText->SetText(FText::FromString(FString::Printf(TEXT("%+.0f%%"), BoostPercent)));

		bool const bTimed = Entry.TimeRemaining >= 0.f && Entry.Duration > 0.f;
		Tile.TimeMeter->SetVisibility(bTimed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		if (bTimed)
		{
			Tile.TimeMeter->SetFill(FMath::Clamp(Entry.TimeRemaining / Entry.Duration, 0.f, 1.f));
		}
	}
}
