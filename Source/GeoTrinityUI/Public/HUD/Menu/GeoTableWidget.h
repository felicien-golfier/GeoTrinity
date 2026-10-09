// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoTableWidget.generated.h"

class UGeoFrameStyle;
class UHorizontalBox;
class USizeBoxSlot;
class UTextBlock;
class UVerticalBox;

/** One column of a UGeoTableWidget. */
USTRUCT(BlueprintType)
struct FGeoTableColumn
{
	GENERATED_BODY()

	/** The caption over the column. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable")
	FText Header;

	/** 0 fills the room the fixed columns leave, shared with every other filling column. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable", meta = (ClampMin = "0", ClampMax = "800"))
	float Width = 0.f;

	/** Where the caption and the cells sit across the column. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable")
	TEnumAsByte<EHorizontalAlignment> Alignment = HAlign_Left;
};

/**
 * Every table of the game: a header line naming the columns, then lines of framed cells scrolling under it. One asset
 * (WBP_Table) carries the look — the cell frame, sizes and gaps — so editing it re-skins every table; a page sets only
 * the Columns of its instance and fills the lines (ClearLines, then AddLine per line).
 * Required in the BP hierarchy: UHorizontalBox "HeaderLine", UVerticalBox "LineBox" (inside a scroll box).
 */
UCLASS()
class GEOTRINITYUI_API UGeoTableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ClearLines();

	/** Appends a line of Cells, one per column in order; the last cell also spans the columns left after it. */
	void AddLine(TArray<UWidget*> const& Cells);

	/** A cell's text, in the Table text role, cut with an ellipsis when its cell is too narrow. */
	UTextBlock* MakeCellText(FText const& Text) const;

protected:
	/** Builds the header from Columns, so the designer previews it. */
	virtual void NativePreConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable")
	TArray<FGeoTableColumn> Columns;

	/** The frame of each cell. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable")
	TObjectPtr<UGeoFrameStyle> CellStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable", meta = (ClampMin = "8", ClampMax = "128"))
	float CellHeight = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable", meta = (ClampMin = "0", ClampMax = "64"))
	float CellPadding = 8.f;

	/** Space between two cells, and between two lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTable", meta = (ClampMin = "0", ClampMax = "64"))
	float CellGap = 4.f;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HeaderLine;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> LineBox;

private:
	/** Appends to Line a box CellHeight high and ColumnCount columns wide from FirstColumn, holding Content. */
	USizeBoxSlot* AddToLine(UHorizontalBox& Line, UWidget* Content, int32 FirstColumn, int32 ColumnCount) const;
};
