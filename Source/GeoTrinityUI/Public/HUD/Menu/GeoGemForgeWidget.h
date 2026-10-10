// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoGemPageWidget.h"
#include "Tool/GeoColor.h"
#include "Types/SlateEnums.h"

#include "GeoGemForgeWidget.generated.h"

class UEditableTextBox;
class UGeoMenuButton;
class UGeoTableWidget;
class UHorizontalBox;
class UVerticalBox;

/**
 * The Forge page of the Gems menu: gems into shards and shards into gems. Left, the tiers: clicking one selects or
 * deselects it, BREAK DOWN SELECTED breaks down every free copy of the selected tiers, and a framed line under it shows
 * the gems that destroys and the shards it gives, with each tier's shard rates under them. Middle, the gem types of the chosen tier with their owned and free counts. Right, the picked type
 * broken down or crafted by a quantity typed in its field or set with -, + and MAX. Free here is what no loadout needs
 * (UGeoGemProfileSave::GetBreakableCount): slotted copies are never broken down. MessageText says what the last action
 * gave.
 * Required in the BP hierarchy: UVerticalBox "GemListBox". Every other part is optional.
 */
UCLASS()
class GEOTRINITYUI_API UGeoGemForgeWidget : public UGeoGemPageWidget
{
	GENERATED_BODY()

public:
	/** Refreshes the Forge page from the current gem profile: tier break-down summary, gem list and picked gem detail. */
	virtual void Refresh() override;

protected:
	/** Wires the buttons. */
	virtual void NativeConstruct() override;

	/** Filled with one selectable break-down row per tier. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> TierBreakBox;

	/** BREAK DOWN SELECTED: breaks down every free copy of the selected tiers. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakAllButton;

	/** How many gems BreakAllButton destroys, in BreakCountColor. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BreakCountText;

	/** How many shards BreakAllButton gives, in BreakShardsColor. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BreakShardsText;

	/** Filled with each tier's break-down and craft rates, its columns tier, break down, craft. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoTableWidget> RateTable;

	/** Filled with one tab per tier. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> TierTabBox;

	/** Filled with one row per gem type of the chosen tier. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> GemListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoGemGlyph> PickedGlyph;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PickedTierText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PickedNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PickedEffectText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PickedCountsText;

	/** The break-down quantity, typed or set by the buttons around it. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> BreakQuantityBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakLessButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakMoreButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakMaxButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakButton;

	/** The craft quantity, typed or set by the buttons around it. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> CraftQuantityBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> CraftLessButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> CraftMoreButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> CraftMaxButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> CraftButton;

	/** What the last break-down or craft gave. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "8", ClampMax = "128"))
	float TierGlyphSize = 34.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "8", ClampMax = "128"))
	float GemGlyphSize = 40.f;

	/** Space between two tier tabs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0", ClampMax = "64"))
	float TierTabGap = 22.f;

	/** {0} owned, {1} slotted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText TierSubFormat = INVTEXT("{0} OWNED \u00B7 {1} SLOTTED");

	/** {0} free copies, {1} the shards they give. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText TierBreakFormat = INVTEXT("\u00D7{0} \u00B7 +{1}");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NoneFreeText = INVTEXT("NONE FREE");

	/** {0} free copies of the selected tiers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText BreakCountFormat = INVTEXT("\u2212{0} GEMS");

	/** {0} the shards breaking the selected tiers down gives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText BreakShardsFormat = INVTEXT("+{0} SHARDS");

	/** The gems about to be destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Break")
	FGeoColorParam BreakCountColor{FLinearColor(1.f, .2f, .25f)};

	/** The shards about to be gained. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Break")
	FGeoColorParam BreakShardsColor{FLinearColor(.75f, .6f, 1.f)};

	/** Alpha of the summary while nothing is selected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Break", meta = (ClampMin = "0", ClampMax = "1"))
	float BreakIdleAlpha = .35f;

	/** {0} the copies owned, on a gem row. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText OwnedCellFormat = INVTEXT("{0} OWNED");

	/** {0} the copies no loadout needs, on a gem row. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText FreeCellFormat = INVTEXT("{0} FREE");

	/** {0} the shards breaking one gem of a tier gives, in the rate table. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText RateBreakFormat = INVTEXT("+{0}");

	/** {0} is the tier's number, {1} its name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText TierFormat = INVTEXT("TIER {0} \u00B7 {1}");

	/** {0} owned, {1} slotted, {2} free. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText CountsFormat = INVTEXT("OWNED {0} \u00B7 SLOTTED {1} \u00B7 FREE {2}");

	/** {0} copies, {1} the shards they give; the quantity shows just above, so the button keeps short. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText BreakFormat = INVTEXT("BREAK +{1}");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NoFreeCopyText = INVTEXT("NO FREE COPY");

	/** {0} copies, {1} the shards they cost; the quantity shows just above, so the button keeps short. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText CraftFormat = INVTEXT("CRAFT \u2212{1}");

	/** {0} the shards the quantity costs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NeedShardsFormat = INVTEXT("NEED {0} SHARDS");

	/** {0} shards gained, {1} copies, {2} what was broken down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText BrokenFormat = INVTEXT("+{0} SHARDS FROM {1} {2}");

	/** {0} copies, {1} the gem, {2} the shards spent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText CraftedFormat = INVTEXT("CRAFTED {0} {1} \u00B7 \u2212{2}");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText AllGemsName = INVTEXT("GEMS");

private:
	/** Breaks down every free copy of the selected tiers. */
	UFUNCTION()
	void HandleBreakAll();

	UFUNCTION()
	void HandleBreakLess();

	UFUNCTION()
	void HandleBreakMore();

	UFUNCTION()
	void HandleBreakMax();

	UFUNCTION()
	void HandleBreakQuantityCommitted(FText const& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleBreak();

	UFUNCTION()
	void HandleCraftLess();

	UFUNCTION()
	void HandleCraftMore();

	UFUNCTION()
	void HandleCraftMax();

	UFUNCTION()
	void HandleCraftQuantityCommitted(FText const& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleCraft();

	void ShowTierBreaks(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	void ShowRates(UGeoGemCatalog const& Catalog);
	void ShowGems(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	void ShowPicked(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);

	/** Whether Tier is selected for BreakAllButton and has a free copy to break down. */
	bool IsBreakSelected(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog, EGeoGemTier Tier) const;

	/** Copies of every gem of Tier the Forge may break down. */
	int32 GetBreakableCount(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog, EGeoGemTier Tier) const;

	/** The most copies of the picked gem the shards pay for, at least 1. */
	int32 GetMaxCraftQuantity() const;

	/** Shown in the middle list. */
	EGeoGemTier ShownTier = EGeoGemTier::Chip;

	/** The tiers BreakAllButton breaks down, picked on the left. */
	TSet<EGeoGemTier> BreakTiers;

	/** Defaults to the shown tier's first gem. */
	FName PickedGem;

	int32 BreakQuantity = 1;
	int32 CraftQuantity = 1;
};
