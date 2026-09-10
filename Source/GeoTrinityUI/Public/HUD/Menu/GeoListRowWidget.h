// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoListRowWidget.generated.h"

class UGeoButton;
class UHorizontalBox;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FGeoListRowClickedSignature);

/** Which of the row's background colours it wears. */
UENUM()
enum class EGeoListRowTint : uint8
{
	/** The row's own colour. */
	Normal,
	/** The colour every other row wears, so a long list reads as separate lines. */
	Alternate,
	/** The line naming the columns, which is not one of the entries under it. */
	Header,
	/** The row of a set that is in play — the open tab of a strip. */
	Selected
};

/**
 * One line of any list in the game. The row holds no data of its own: whichever list builds it fills it with
 * columns and, for a clickable row, binds OnClicked. Every list instantiates the same Blueprint (WBP_ListRow), so
 * the button style it wears and the column font, colour and padding below are the skin of all of them.
 * Columns take a share of the row rather than a fixed width, so a list always spans its panel and every row lines
 * up with the header above it as long as they are given the same shares.
 * Required in the BP hierarchy: UGeoButton "RowButton" holding UHorizontalBox "ColumnsBox".
 */
UCLASS()
class GEOTRINITYUI_API UGeoListRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGeoListRowWidget(FObjectInitializer const& ObjectInitializer);

	/** Fires on click, for a row the list made selectable. */
	FGeoListRowClickedSignature OnClicked;

	/** Appends a column holding Content. Weight 0 sizes it to its content, otherwise it is its share of the row. */
	void AddColumn(UWidget* Content, float Weight);

	/** Appends a column of text in the row's own font. */
	void AddTextColumn(FText const& Text, float Weight);

	/** A text block in the row's own font, for a caller building a column of more than text. */
	UTextBlock* MakeColumnText(FText const& Text);

	/** A row that is only read, rather than clicked, keeps its look but takes no input. */
	void SetSelectable(bool bSelectable);

	/**
	 * Paints the row background in the colour of its role. The colour goes on the button style's own brush rather
	 * than through its background colour, which only multiplies a skin authored transparent.
	 */
	void SetTint(EGeoListRowTint Tint);

protected:
	/** Wires RowButton and makes the columns span the row. */
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoButton> RowButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> ColumnsBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	FSlateFontInfo ColumnFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	FSlateColor ColumnColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	FMargin ColumnPadding;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	FLinearColor NormalColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	FLinearColor AlternateColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	FLinearColor HeaderColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	FLinearColor SelectedColor;

private:
	UFUNCTION()
	void HandleClicked();
};
