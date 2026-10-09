// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoCharacterSheetWidget.generated.h"

class APlayableCharacter;
enum class EGeoTextRole : uint8;
class UGeoAbilityCardWidget;
class UGeoAbilityDetailWidget;
class UGeoGemCatalog;
class UGeoGemProfileSave;
class UGeoMenuButton;
class UGeoMeter;
class UGeoShape;
class UGeoTableWidget;
class UHorizontalBox;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;
struct FOnAttributeChangeData;

/** How a stat's value reads on the character sheet. */
UENUM(BlueprintType)
enum class EGeoStatFormat : uint8
{
	/** A whole number: 1000. */
	Number,
	/** A multiplier: ×1.20. */
	Multiplier,
	/** A share as a percentage: 0.15 reads 15%. */
	Percent
};

/** One stat line of the character sheet. */
USTRUCT(BlueprintType)
struct FGeoSheetStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	EGeoStatFormat Format = EGeoStatFormat::Number;
};

/**
 * The local player's class at a glance, rebuilt on each open: its shape, name, role and level with the XP to the next,
 * the other classes' levels, every stat live with what the slotted gems and the buffs now on the player add to it, this
 * fight's damage, healing and damage taken (now, average, peak, total, refreshed while shown), the class's abilities
 * (AbilityRowClass, the ability card in a compact layout, its description cut short), the slotted gems with their Core
 * effects and rules, and the gems and shards owned. Opened from the pause menu, or held open with Tab outside any
 * menu; GEMS opens the Gems page, opening the pause menu first from the Tab sheet.
 * Picking an ability opens its whole description and stats in DetailWidget, the drawer sliding in from the right over
 * DetailScrim; a click outside the drawer, or the back input, closes it.
 * Every part is optional.
 */
UCLASS()
class GEOTRINITYUI_API UGeoCharacterSheetWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

public:
	/** Lists the stats every class has, in the order the sheet shows them. */
	UGeoCharacterSheetWidget(FObjectInitializer const& ObjectInitializer);

	/** Fills the sheet from the class, the profile and the fight as they are now, on each show: a page it opened may
	 * have changed the gems. */
	virtual void OnPageShown() override;

protected:
	/** Wires GEMS, follows the stats and refreshes this fight's numbers every FightRefreshInterval; a sheet outside any
	 * menu (the one held open with Tab) fills itself here, as no menu will show it. */
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	/** The first ability. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Closes the open details, else goes back. */
	virtual bool HandleBackAction() override;
	/** Closes the open details: only a click outside the drawer reaches here while it is open. */
	virtual FReply NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoAbilityDetailWidget> DetailWidget;

	/** Dims the sheet while the details are open, and takes the click that closes them. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DetailScrim;

	/** Opens the Gems menu. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> GemLoadoutButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoShape> ClassShape;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClassNameText;

	/** "TANK, player name". */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	/** What the next level opens, or that the class is at the max level. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextLevelText;

	/** Level over the max level, in the class colour; its style cuts it in one pip per level. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMeter> LevelPips;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMeter> XpMeter;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> XpText;

	/** Filled with each other class's shape and level. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> OtherClassesBox;

	/** Filled with one line per Stats entry: its label, its live value, what the slotted gems add and what the buffs now
	 * on the player add; then the deploy ability's charges, what the gems add to them. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoTableWidget> StatTable;

	/** Filled with this fight's damage, healing and damage taken: per second now and on average, the peak and the
	 * total. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoTableWidget> FightTable;

	/** Filled with one AbilityRowClass per ability. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> AbilityBox;

	/** "36 / 38": sockets filled over sockets open. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlottedText;

	/** Filled with one chip per slotted gem type: its glyph and its count. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> GemBox;

	/** Filled with the name of each slotted Core, its rule under it. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> CoreEffectBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShardsText;

	/** Gems owned per tier. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OwnedText;

	/** The stat lines, top to bottom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	TArray<FGeoSheetStat> Stats;

	/** One ability: WBP_AbilityRow, the ability card in a compact layout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	TSubclassOf<UGeoAbilityCardWidget> AbilityRowClass;

	/** A gem bonus on a stat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FLinearColor GemBonusColor = FLinearColor(1.f, .58f, .03f, 1.f);

	/** What the buffs add to a stat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FLinearColor BuffBonusColor = FLinearColor(.36f, .78f, 1.f, 1.f);

	/** The dash standing in for no bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FLinearColor NoBonusColor = FLinearColor(.11f, .08f, .2f, 1.f);

	/** Size of each other class's shape, by its level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "4", ClampMax = "64"))
	float OtherClassShapeSize = 16.f;

	/** Space between the items of a row: after an other class's level, after a gem chip. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0", ClampMax = "64"))
	float ItemGap = 14.f;

	/** Space between a shape or a glyph and the text it labels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0", ClampMax = "64"))
	float LabelGap = 8.f;

	/** Space under each gem chip and Core name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0", ClampMax = "64"))
	float LineGap = 8.f;

	/** Space under each ability. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0", ClampMax = "64"))
	float AbilityGap = 18.f;

	/** Size of a slotted gem's glyph. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "8", ClampMax = "64"))
	float GemGlyphSize = 22.f;

	/** {0} is the class role, {1} the player name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText SubtitleFormat = INVTEXT("{0}  \u00B7  {1}");

	/** {0} is an other class's level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText OtherClassLevelFormat = INVTEXT("LV {0}");

	/** {0} is a bonus on a multiplier or a share, in signed percentage points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText BonusFormat = INVTEXT("{0}%");

	/** Stands in for no bonus on a stat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText NoBonusText = INVTEXT("\u2014");

	/** {0} is the sockets filled, {1} the sockets open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText SlottedFormat = INVTEXT("{0} / {1}");

	/** {0} is how many copies of a gem are slotted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText GemCountFormat = INVTEXT("\u00D7{0}");

	/** {0} is the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText LevelFormat = INVTEXT("LEVEL {0}  / {1}");

	/** {0} is the next level, {1} the sockets it opens. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText NextLevelFormat = INVTEXT("NEXT  \u00B7  LV {0}  \u00B7  +{1} SOCKETS");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText MaxLevelText = INVTEXT("MAX LEVEL");

	/** {0} is the XP earned, {1} the XP the level takes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText XpFormat = INVTEXT("{0} / {1} XP");

	/** {0} chips, {1} cuts, {2} prisms, {3} cores. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText OwnedFormat = INVTEXT("CHIPS {0}  \u00B7  CUTS {1}  \u00B7  PRISMS {2}  \u00B7  CORES {3}");

	/** Seconds between two refreshes of this fight's numbers while the sheet is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0.05", ClampMax = "5"))
	float FightRefreshInterval = .5f;

	/** The stat line of the deploy ability's charges. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText DeployStacksLabel = INVTEXT("Deploy stacks");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText FightDamageLabel = INVTEXT("Damage");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText FightHealingLabel = INVTEXT("Healing");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText FightTakenLabel = INVTEXT("Taken");

	/** Font size of the texts the sheet builds itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "6", ClampMax = "64"))
	int32 RowTextSize = 15;

private:
	UFUNCTION()
	void HandleAbilitySelected(UGeoAbilityCardWidget* Row);

	void CloseDetail();

	UFUNCTION()
	void HandleGems();

	void ShowIdentity(EPlayerClass PlayerClass, UGeoGemProfileSave const* Profile);
	/** Level, XP, what the next level opens and the other classes' levels. */
	void ShowLevel(EPlayerClass PlayerClass, UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	/** Each stat's live value, split into its base, what the gem effect adds and what every other active effect adds;
	 * then the deploy ability's charges. */
	void ShowStats();
	void HandleStatChanged(FOnAttributeChangeData const& Data);
	/** A StatTable cell reading Bonus as Format does, signed and in Color, or NoBonusText when there is none. */
	UTextBlock* MakeBonusCell(EGeoStatFormat Format, float Bonus, FLinearColor const& Color) const;
	void ShowFight();
	void ShowAbilities(APlayableCharacter const& PlayableCharacter);
	void ShowGems(EPlayerClass PlayerClass, UGeoGemProfileSave const* Profile);

	/** A text the sheet builds, in Role at RowTextSize. */
	UTextBlock* MakeText(EGeoTextRole Role, FText const& Text) const;

	FTimerHandle FightRefreshTimer;

	/** The ability whose details are open; null when closed. */
	UPROPERTY(Transient)
	TObjectPtr<UGeoAbilityCardWidget> SelectedRow;
};
