// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoGemPageWidget.h"

#include "GeoGemForgeWidget.generated.h"

class UGeoMenuButton;
class UHorizontalBox;
class UVerticalBox;

/**
 * The Forge page of the Gems menu: gems into shards and shards into gems. Left, one click breaks down every free copy of
 * a tier, or of every Chip, Cut and Prism at once, with each tier's shard rates under them. Middle, the gem types of the
 * chosen tier with their owned and free counts. Right, the picked type broken down or crafted by a quantity set with
 * -, + and MAX. Free here is what no loadout needs (UGeoGemProfileSave::GetBreakableCount): slotted copies are never
 * broken down. MessageText says what the last action gave.
 * Required in the BP hierarchy: UVerticalBox "GemListBox". Every other part is optional.
 */
UCLASS()
class GEOTRINITYUI_API UGeoGemForgeWidget : public UGeoGemPageWidget
{
	GENERATED_BODY()

public:
	virtual void Refresh() override;

protected:
	/** Wires the buttons. */
	virtual void NativeConstruct() override;

	/** Filled with one break-down row per tier. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> TierBreakBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakAllButton;

	/** What BREAK DOWN ALL FREE would give. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BreakAllText;

	/** Filled with each tier's break-down and craft rates. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RateBox;

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

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BreakQuantityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakLessButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakMoreButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakMaxButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> BreakButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CraftQuantityText;

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

	/** {0} owned, {1} slotted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText TierSubFormat = INVTEXT("{0} OWNED \u00B7 {1} SLOTTED");

	/** {0} free copies, {1} the shards they give. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText TierBreakFormat = INVTEXT("\u00D7{0} \u00B7 +{1}");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NoneFreeText = INVTEXT("NONE FREE");

	/** {0} free Chips, Cuts and Prisms, {1} the shards they give. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText BreakAllFormat = INVTEXT("CHIPS, CUTS & PRISMS \u00B7 {0} GEMS \u00B7 +{1} SHARDS");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NothingFreeText = INVTEXT("NOTHING FREE TO BREAK DOWN");

	/** The rate table's header: tier, break down, craft. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	TArray<FText> RateHeaders = {INVTEXT("TIER"), INVTEXT("BREAK DOWN"), INVTEXT("CRAFT")};

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
	UFUNCTION()
	void HandleBreakAll();

	UFUNCTION()
	void HandleBreakLess();

	UFUNCTION()
	void HandleBreakMore();

	UFUNCTION()
	void HandleBreakMax();

	UFUNCTION()
	void HandleBreak();

	UFUNCTION()
	void HandleCraftLess();

	UFUNCTION()
	void HandleCraftMore();

	UFUNCTION()
	void HandleCraftMax();

	UFUNCTION()
	void HandleCraft();

	void ShowTierBreaks(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	void ShowRates(UGeoGemCatalog const& Catalog);
	void ShowGems(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	void ShowPicked(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);

	/** Breaks every breakable copy of Tiers down, saying what it gave as Name. */
	void BreakTiers(TArray<EGeoGemTier> const& Tiers, FText const& Name);

	/** Copies of every gem of Tier the Forge may break down. */
	int32 GetBreakableCount(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog, EGeoGemTier Tier) const;

	/** The most copies of the picked gem the shards pay for, at least 1. */
	int32 GetMaxCraftQuantity() const;

	/** Shown in the middle list. */
	EGeoGemTier ShownTier = EGeoGemTier::Chip;

	/** Defaults to the shown tier's first gem. */
	FName PickedGem;

	int32 BreakQuantity = 1;
	int32 CraftQuantity = 1;
};
