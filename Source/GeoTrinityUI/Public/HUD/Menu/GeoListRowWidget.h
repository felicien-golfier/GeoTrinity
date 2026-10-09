// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "HUD/Style/GeoUITheme.h"
#include "Types/SlateEnums.h"

#include "GeoListRowWidget.generated.h"

class UGeoButton;
class UGeoFrame;
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
	/** A line framing the entries rather than one of them: the one naming the columns, or a strip filtering them. */
	Header,
	/** The row of a set that is in play — the open tab of a strip. */
	Selected
};

/**
 * One line of any list in the game. The row holds no data of its own: whichever list builds it fills it with
 * columns and, for a clickable row, binds OnClicked. Every list instantiates the same Blueprint (WBP_ListRow), so
 * the button style it wears and the column text roles, padding and tints below are the skin of all of them. Column
 * text wears ColumnTextRole from the UI theme, or HeaderTextRole on a header row.
 * Columns take a share of the row rather than a fixed width, so a list always spans its panel and every row lines
 * up with the header above it as long as they are given the same shares.
 * Required in the BP hierarchy: UGeoButton "RowButton" holding UHorizontalBox "ColumnsBox". Optional: a UGeoFrame
 * "RowFrame" around RowButton, active while the row is the selected one.
 */
UCLASS()
class GEOTRINITYUI_API UGeoListRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Seeds the column padding and the four background tint colors for each row role. */
	UGeoListRowWidget(FObjectInitializer const& ObjectInitializer);

	/** Fires on click, for a row the list made selectable. */
	FGeoListRowClickedSignature OnClicked;

	/** Fires on a double click in place of its second click, for a row the list made selectable. Unbound, a double
	 * click is two clicks. */
	FGeoListRowClickedSignature OnDoubleClicked;

	/** Appends a column holding Content. Weight 0 sizes it to its content, otherwise it is its share of the row. */
	void AddColumn(UWidget* Content, float Weight);

	/** Appends a column of text in the row's own font. */
	void AddTextColumn(FText const& Text, float Weight);

	/** A text block in the row's own text role, for a caller building a column of more than text. */
	UTextBlock* MakeColumnText(FText const& Text);

	/** A row that is only read, rather than clicked, keeps its look but takes no input and never goes active. */
	void SetSelectable(bool bSelectable);

	/**
	 * Paints the row background in the colour of its role, and sets RowFrame active on the selected row. The colour
	 * goes on the button style's own brush rather than through its background colour, which only multiplies a skin
	 * authored transparent. Set before adding columns: a header row's columns take HeaderTextRole.
	 */
	void SetTint(EGeoListRowTint InTint);

	/** Draws RowFrame's line and glow in Color, for a row that stands for something with its own colour. */
	void SetFrameTint(FLinearColor const& Color);

	/** When a click fires: on release by default, on press for a row that a press elsewhere may rebuild under it. */
	void SetClickMethod(EButtonClickMethod::Type ClickMethod);

	/** Gives the row's button the user's focus, as a list rebuilt under the gamepad does for the row it stood on. */
	void FocusRow();

protected:
	/** Wires RowButton and makes the columns span the row. */
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoButton> RowButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> ColumnsBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoFrame> RowFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	EGeoTextRole ColumnTextRole = EGeoTextRole::Body;

	/** Text role of a header row's columns: the line naming the columns, or a strip filtering them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoListRow|Appearance")
	EGeoTextRole HeaderTextRole = EGeoTextRole::Label;

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

	void HandleDoubleClicked();

	EGeoListRowTint Tint = EGeoListRowTint::Normal;
};
