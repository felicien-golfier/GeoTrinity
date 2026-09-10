// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoListRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "HUD/Menu/GeoButton.h"
#include "Styling/CoreStyle.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoListRowWidget::UGeoListRowWidget(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	ColumnFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18.f);
	ColumnColor = FSlateColor(FLinearColor::White);
	ColumnPadding = FMargin(6.f, 3.f);
	// Overlays on the panel the list sits on rather than colours of their own, so a row reads against whatever skin
	// the frame wears.
	NormalColor = FLinearColor(1.f, 1.f, 1.f, .03f);
	AlternateColor = FLinearColor(1.f, 1.f, 1.f, .1f);
	HeaderColor = FLinearColor(0.f, 0.f, 0.f, .45f);
	SelectedColor = FLinearColor(.25f, .55f, 1.f, .55f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RowButton->OnClicked.AddUniqueDynamic(this, &UGeoListRowWidget::HandleClicked);

	// A button centers its content; the columns are the row, so they span it whatever the style was authored with.
	if (UButtonSlot* const ContentSlot = Cast<UButtonSlot>(ColumnsBox->Slot))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListRowWidget::AddColumn(UWidget* Content, float const Weight)
{
	// What does not fit its share is cut there, rather than pushing the columns after it out of line.
	Content->SetClipping(EWidgetClipping::ClipToBounds);

	UHorizontalBoxSlot* const Column = ColumnsBox->AddChildToHorizontalBox(Content);
	Column->SetPadding(ColumnPadding);
	Column->SetVerticalAlignment(VAlign_Center);
	if (Weight > 0.f)
	{
		FSlateChildSize Share(ESlateSizeRule::Fill);
		Share.Value = Weight;
		Column->SetSize(Share);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListRowWidget::AddTextColumn(FText const& Text, float const Weight)
{
	AddColumn(MakeColumnText(Text), Weight);
}

// ---------------------------------------------------------------------------------------------------------------------
UTextBlock* UGeoListRowWidget::MakeColumnText(FText const& Text)
{
	UTextBlock* const Column = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Column->SetText(Text);
	Column->SetFont(ColumnFont);
	Column->SetColorAndOpacity(ColumnColor);
	return Column;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListRowWidget::SetSelectable(bool const bSelectable)
{
	RowButton->SetVisibility(bSelectable ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListRowWidget::SetTint(EGeoListRowTint const Tint)
{
	FButtonStyle Style = RowButton->GetStyle();
	switch (Tint)
	{
	case EGeoListRowTint::Alternate:
		Style.Normal.TintColor = AlternateColor;
		break;
	case EGeoListRowTint::Header:
		Style.Normal.TintColor = HeaderColor;
		break;
	case EGeoListRowTint::Selected:
		Style.Normal.TintColor = SelectedColor;
		break;
	default:
		Style.Normal.TintColor = NormalColor;
		break;
	}
	RowButton->SetStyle(Style);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListRowWidget::HandleClicked()
{
	OnClicked.Broadcast();
}
