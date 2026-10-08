// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoCharacterSheetWidget.generated.h"

class APlayableCharacter;
enum class EGeoTextRole : uint8;
class UGeoAbilityCardWidget;
class UGeoGemCatalog;
class UGeoGemProfileSave;
class UGeoMenuButton;
class UGeoMeter;
class UGeoShape;
class UHorizontalBox;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoCharacterSheetSignature);

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
 * the other classes' levels, every stat with what the slotted gems add to it, this fight's numbers, the class's
 * abilities (AbilityRowClass, the ability card in a compact layout), the slotted gems with their Core effects, and the
 * gems and shards owned. Opened from the pause menu; ABILITY DETAILS asks it for the abilities page, GEMS for the Gems
 * menu.
 * Required in the BP hierarchy: UGeoMenuButton "AbilityDetailsButton". Every other part is optional.
 */
UCLASS()
class GEOTRINITYUI_API UGeoCharacterSheetWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	/** Lists the stats every class has, in the order the sheet shows them. */
	UGeoCharacterSheetWidget(FObjectInitializer const& ObjectInitializer);

	/** Fills the sheet from the class, the profile and the fight as they are now. */
	void Refresh();

	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoCharacterSheetSignature OnClosed;

	/** ABILITY DETAILS was pressed. */
	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoCharacterSheetSignature OnOpenAbilityDetails;

	/** GEMS was pressed. */
	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoCharacterSheetSignature OnOpenGems;

protected:
	/** Wires the buttons and fills the sheet. */
	virtual void NativeConstruct() override;
	virtual UWidget* GetInitialFocusWidget() const override;
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> AbilityDetailsButton;

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

	/** Filled with one line per Stats entry: label, value, gem bonus. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> StatBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FightDpsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FightHpsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FightTakenText;

	/** Filled with one AbilityRowClass per ability. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> AbilityBox;

	/** "36 / 38": sockets filled over sockets open. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlottedText;

	/** Filled with one chip per slotted gem type: its glyph and its count. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> GemBox;

	/** Filled with the name of each slotted Core. */
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

	/** The dash standing in for no gem bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FLinearColor NoBonusColor = FLinearColor(.11f, .08f, .2f, 1.f);

	/** Size of each other class's shape, by its level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "4", ClampMax = "64"))
	float OtherClassShapeSize = 16.f;

	/** Width the gem bonus column keeps, so the values line up whatever the bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0", ClampMax = "256"))
	float BonusColumnWidth = 74.f;

	/** Space between the items of a row: after an other class's level, before a gem bonus, after a gem chip. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0", ClampMax = "64"))
	float ItemGap = 14.f;

	/** Space between a shape or a glyph and the text it labels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "0", ClampMax = "64"))
	float LabelGap = 8.f;

	/** Space under each stat line, gem chip and Core name. */
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

	/** {0} is the gem bonus on a stat, a signed percentage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet")
	FText BonusFormat = INVTEXT("{0}%");

	/** Stands in for no gem bonus on a stat. */
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

	/** Font size of the texts the sheet builds itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacterSheet", meta = (ClampMin = "6", ClampMax = "64"))
	int32 RowTextSize = 15;

private:
	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleAbilityDetails();

	UFUNCTION()
	void HandleGems();

	void ShowIdentity(EPlayerClass PlayerClass, UGeoGemProfileSave const* Profile);
	/** Level, XP, what the next level opens and the other classes' levels. */
	void ShowLevel(EPlayerClass PlayerClass, UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	void ShowStats(EPlayerClass PlayerClass, UGeoGemProfileSave const* Profile);
	void ShowFight();
	void ShowAbilities(APlayableCharacter const& PlayableCharacter);
	void ShowGems(EPlayerClass PlayerClass, UGeoGemProfileSave const* Profile);

	/** A text the sheet builds, in Role at RowTextSize. */
	UTextBlock* MakeText(EGeoTextRole Role, FText const& Text) const;
};
