// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Gem/GeoGemTypes.h"
#include "HUD/Menu/GeoListRowWidget.h"

#include "GeoGemPageWidget.generated.h"

enum class EGeoTextRole : uint8;
class UGeoGemCatalog;
class UGeoGemGlyph;
class UGeoGemProfileSave;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FGeoGemProfileChangedSignature);

/**
 * A page of the Gems menu (UGeoGemsWidget): what both the Loadout and the Forge build their rows with, and the one way
 * they change the local player's gem profile, through CommitChanges, which saves it, sends it to the server and tells
 * the menu so the shard count follows.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoGemPageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows the profile as it is now. */
	virtual void Refresh() PURE_VIRTUAL(UGeoGemPageWidget::Refresh, );

	/** After every change this page made to the profile. */
	FGeoGemProfileChangedSignature OnProfileChanged;

	/** Every tier, lowest first. */
	static TArray<EGeoGemTier> const& GetTiers();

protected:
	UGeoGemProfileSave* GetProfile() const;

	/** Saves and sends the profile, then refreshes the page and broadcasts OnProfileChanged. */
	void CommitChanges();

	/** A row of RowClass in Tint, OnClicked calling Picked; a null Picked makes a row that only reads. */
	UGeoListRowWidget* MakeRow(EGeoListRowTint Tint, TFunction<void()> Picked);

	/** A text the page builds, in Role. */
	UTextBlock* MakeText(EGeoTextRole Role, FText const& Text) const;

	/** GemId's glyph at GlyphSize, in its stat colour. */
	UGeoGemGlyph* MakeGemGlyph(UGeoGemCatalog const& Catalog, FName GemId, float GlyphSize) const;

	/** Tier's name: CHIP, or CHIPS when bPlural. */
	FText GetTierName(EGeoGemTier Tier, bool bPlural) const;

	/** The gem rows, tabs and lines of the page: WBP_ListRow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TSubclassOf<UGeoListRowWidget> RowClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TMap<EGeoGemTier, FText> TierNames = {{EGeoGemTier::Chip, INVTEXT("CHIP")},
										  {EGeoGemTier::Cut, INVTEXT("CUT")},
										  {EGeoGemTier::Prism, INVTEXT("PRISM")},
										  {EGeoGemTier::Core, INVTEXT("CORE")}};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TMap<EGeoGemTier, FText> TierPluralNames = {{EGeoGemTier::Chip, INVTEXT("CHIPS")},
												{EGeoGemTier::Cut, INVTEXT("CUTS")},
												{EGeoGemTier::Prism, INVTEXT("PRISMS")},
												{EGeoGemTier::Core, INVTEXT("CORES")}};
};
