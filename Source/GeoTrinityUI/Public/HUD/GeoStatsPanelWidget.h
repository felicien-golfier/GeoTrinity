// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoStatsPanelWidget.generated.h"

class AGeoArena;
class AGeoPlayerState;
class UGeoIcon;
class UGridPanel;
class UTextBlock;
class UWidget;

/** One figure of the stat counter. */
UENUM(BlueprintType)
enum class EGeoStatColumn : uint8
{
	DamageNow,
	DamageAverage,
	DamagePeak,
	DamageTotal,
	HealingNow,
	HealingAverage,
	HealingPeak,
	HealingTotal,
	DamageTaken,
	DamageModifier,
	HealingModifier,
	Armor,
	Speed
};

/** The heading a run of stat columns shares. */
UENUM(BlueprintType)
enum class EGeoStatGroup : uint8
{
	Damage,
	Healing,
	DamageTaken,
	Modifiers
};

/** The widgets of one player's line, kept to write its figures in place every tick. */
USTRUCT()
struct FGeoStatsRow
{
	GENERATED_BODY()

	TWeakObjectPtr<AGeoPlayerState> Player;

	/** One per shown column, in its order. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> Cells;
};

/**
 * The stat counter: every player's combat figures as one bare table over the arena, the owning player first and bright,
 * the others dimmed. Short, it shows the fight's time and, per player, damage and healing per second now and over the
 * fight (ø) and the damage taken; full, it adds each one's peak and total and the player's modifiers. The key bound to
 * AGeoPlayerController::ToggleStatsDetailAction switches between the two, and the hint under the table names it. Built
 * in C++ into StatsGrid: rebuilt when the players or the detail change, its figures written in place every tick.
 * Shown while the player's Show Combat Stats setting is on.
 * Required in the BP hierarchy: UGridPanel "StatsGrid". Optional: UTextBlock "ToggleHintText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoStatsPanelWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Lists both column sets and their labels. */
	UGeoStatsPanelWidget(FObjectInitializer const& ObjectInitializer);

protected:
	/** Shows or hides the panel from the setting, and follows its changes and the detail key. */
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	/** Rebuilds the table when the players changed, then writes every figure. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGridPanel> StatsGrid;

	/** "[C] MORE", "[C] LESS". */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ToggleHintText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Columns")
	TArray<EGeoStatColumn> ShortColumns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Columns")
	TArray<EGeoStatColumn> FullColumns;

	/** Over each column in the full table. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Columns")
	TMap<EGeoStatColumn, FText> ColumnLabels;

	/** Heading each run of columns; a group with no icon is headed by its label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Columns")
	TMap<EGeoStatGroup, TObjectPtr<UGeoIcon>> GroupIcons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Columns")
	TMap<EGeoStatGroup, FText> GroupLabels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Columns")
	TObjectPtr<UGeoIcon> FightTimeIcon;

	/** A figure that counts: the "now" ones and the damage taken. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look")
	FLinearColor ValueColor = FLinearColor(.9f, .85f, 1.f, 1.f);

	/** Averages, peaks, totals, modifiers, the fight's time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look")
	FLinearColor SecondaryColor = FLinearColor(.45f, .37f, .7f, 1.f);

	/** A figure at nothing: no damage yet, a modifier at its neutral value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look")
	FLinearColor ZeroColor = FLinearColor(.075f, .058f, .16f, 1.f);

	/** Another player's name; the owning player's wears ValueColor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look")
	FLinearColor NameColor = FLinearColor(.48f, .38f, .75f, 1.f);

	/** Under each heading. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look")
	FLinearColor RuleColor = FLinearColor(.45f, .2f, 1.f, .18f);

	/** Keeps the bare figures legible over the arena. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look")
	FLinearColor ShadowColor = FLinearColor(.0012f, .0012f, .003f, 1.f);

	/** The other players' lines, against the owning player's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "0", ClampMax = "1"))
	float OtherPlayerOpacity = .72f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "4", ClampMax = "64"))
	int32 ValueSize = 18;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "4", ClampMax = "64"))
	int32 SecondarySize = 15;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "4", ClampMax = "64"))
	int32 NameSize = 15;

	/** Column labels, the MODIFIERS heading and the hint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "4", ClampMax = "64"))
	int32 LabelSize = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "4", ClampMax = "64"))
	float IconSize = 18.f;

	/** The class shape at the head of a line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "4", ClampMax = "64"))
	float ShapeSize = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "0", ClampMax = "64"))
	float ColumnGap = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "0", ClampMax = "64"))
	float RowGap = 6.f;

	/** Letters of a player's name the table keeps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Look", meta = (ClampMin = "1", ClampMax = "32"))
	int32 NameLength = 5;

	/** {0} is a fight average. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Text")
	FText AverageFormat = INVTEXT("\u00F8{0}");

	/** {0} is the key that shows the full table. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Text")
	FText MoreFormat = INVTEXT("[{0}] MORE");

	/** {0} is the key that shows the short table. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoStats|Text")
	FText LessFormat = INVTEXT("[{0}] LESS");

private:
	void ShowPanel(bool bShow);
	void ToggleDetail();

	/** The owning player first, then the others in the game state's order. */
	TArray<AGeoPlayerState*> GetPlayers() const;

	/** Lays out the headings and one line per player for the columns shown. */
	void BuildTable(TArray<AGeoPlayerState*> const& Players);

	/** The headings: the fight's time, then a heading per run of columns, then each column's label when full. */
	int32 BuildHeadings(TArray<EGeoStatColumn> const& Columns);

	/** The figures of Row's player, coloured by what they say. */
	void WriteRow(FGeoStatsRow const& Row, TArray<EGeoStatColumn> const& Columns) const;

	/** A text the table builds, in the theme's Mono role at Size and Color, with the shadow. */
	UTextBlock* MakeText(FText const& Text, int32 Size, FLinearColor const& Color) const;

	/** Adds Content to StatsGrid at Row and Column, spanning ColumnSpan, its gap after it. */
	void AddCell(UWidget* Content, int32 Row, int32 Column, int32 ColumnSpan = 1,
				 EHorizontalAlignment Alignment = HAlign_Right) const;

	/** The key bound to the detail toggle, or empty when none is. */
	FText GetToggleKeyName() const;

	TArray<EGeoStatColumn> const& GetShownColumns() const;

	static EGeoStatGroup GetGroup(EGeoStatColumn Column);

	bool bFullTable = false;

	/** Who the table was built for, to rebuild it when someone joins, leaves or swaps class. */
	TArray<TPair<TWeakObjectPtr<AGeoPlayerState>, uint8>> TablePlayers;

	UPROPERTY()
	TArray<FGeoStatsRow> Rows;

	UPROPERTY()
	TObjectPtr<UTextBlock> FightTimeText;

	/** The arena last fought, kept once its fight ends so the fight time holds the final time. */
	TWeakObjectPtr<AGeoArena> Arena;
};
