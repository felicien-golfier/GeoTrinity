// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoTableWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HUD/Style/GeoFrame.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTableWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	HeaderLine->ClearChildren();
	for (int32 Index = 0; Index < Columns.Num(); ++Index)
	{
		UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		UGeoUITheme::ApplyTextStyle(Caption, EGeoTextRole::Label);
		Caption->SetText(Columns[Index].Header);
		USizeBoxSlot* CaptionSlot = AddToLine(*HeaderLine, Caption, Index, 1);
		CaptionSlot->SetPadding(FMargin(CellPadding, 0.f));
		CaptionSlot->SetHorizontalAlignment(Columns[Index].Alignment);
		CaptionSlot->SetVerticalAlignment(VAlign_Center);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTableWidget::ClearLines()
{
	LineBox->ClearChildren();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTableWidget::AddLine(TArray<UWidget*> const& Cells)
{
	if (!ensureMsgf(Cells.Num() <= Columns.Num(), TEXT("%hs: %d cells on a line of %s, which has %d columns"),
					__FUNCTION__, Cells.Num(), *GetName(), Columns.Num()))
	{
		return;
	}

	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		UGeoFrame* Cell = WidgetTree->ConstructWidget<UGeoFrame>(UGeoFrame::StaticClass());
		Cell->SetFrameStyle(CellStyle);
		Cell->SetPadding(FMargin(CellPadding, 0.f));
		Cell->SetHorizontalAlignment(Columns[Index].Alignment);
		Cell->SetVerticalAlignment(VAlign_Center);
		Cell->SetClipping(EWidgetClipping::ClipToBounds);
		Cell->SetContent(Cells[Index]);
		bool const bLastCell = Index == Cells.Num() - 1;
		AddToLine(*Line, Cell, Index, bLastCell ? Columns.Num() - Index : 1);
	}
	LineBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 0.f, 0.f, CellGap));
}

// ---------------------------------------------------------------------------------------------------------------------
UTextBlock* UGeoTableWidget::MakeCellText(FText const& Text) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UGeoUITheme::ApplyTextStyle(TextBlock, EGeoTextRole::Table);
	TextBlock->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	TextBlock->SetText(Text);
	return TextBlock;
}

// ---------------------------------------------------------------------------------------------------------------------
USizeBoxSlot* UGeoTableWidget::AddToLine(UHorizontalBox& Line, UWidget* Content, int32 const FirstColumn,
										 int32 const ColumnCount) const
{
	float Width = CellGap * (ColumnCount - 1);
	bool bFills = false;
	for (int32 Index = FirstColumn; Index < FirstColumn + ColumnCount; ++Index)
	{
		Width += Columns[Index].Width;
		bFills |= Columns[Index].Width <= 0.f;
	}

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetHeightOverride(CellHeight);
	if (!bFills)
	{
		Box->SetWidthOverride(Width);
	}

	UHorizontalBoxSlot* LineSlot = Line.AddChildToHorizontalBox(Box);
	LineSlot->SetPadding(FMargin(FirstColumn > 0 ? CellGap : 0.f, 0.f, 0.f, 0.f));
	if (bFills)
	{
		LineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	return Cast<USizeBoxSlot>(Box->SetContent(Content));
}
