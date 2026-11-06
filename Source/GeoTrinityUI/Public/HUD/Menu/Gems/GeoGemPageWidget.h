// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Gem/GeoGemTypes.h"
#include "HUD/Interface/GeoMenuGuideGoal.h"
#include "HUD/Menu/Frame/GeoMenuGuide.h"
#include "HUD/Menu/List/GeoListRowWidget.h"

#include "GeoGemPageWidget.generated.h"

enum class EGeoTextRole : uint8;
class UGeoGemCatalog;
class UGeoGemGlyph;
class UGeoGemProfileSave;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FGeoGemProfileChangedSignature);

/**
 * A page of the Gems menu (UGeoGemsWidget): what both the Loadout and the Forge build their rows with, and the one way
 * they change the local player's gem profile, through CommitChanges (build edits: saves, sends that class's loadout to the
 * server) or SaveChanges (Forge: saves only), which then tell the menu so the shard count follows.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoGemPageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows the profile as it is now. */
	virtual void Refresh() PURE_VIRTUAL(UGeoGemPageWidget::Refresh, );

	/** The control the menu guide points at on this page for Goal; none when the goal is not for this page. */
	virtual FGeoMenuGuideTarget GetGuideTarget(EGeoMenuGuideGoal /*Goal*/) const { return FGeoMenuGuideTarget(); }

	/** Opens the tab the path of Goal starts on (`UGeoMenuPageWidget::PrepareForGuide`). */
	virtual void PrepareForGuide(EGeoMenuGuideGoal /*Goal*/) {}

	/** After every change this page made to the profile. */
	FGeoGemProfileChangedSignature OnProfileChanged;

	/** Every tier, lowest first. */
	static TArray<EGeoGemTier> const& GetTiers();

protected:
	UGeoGemProfileSave* GetProfile() const;

	/** True while a tutorial guide (EquipChip, ForgeChip) runs and GemId is not the one chip it teaches (Power): the
	 * page refuses the pick. */
	bool IsGemGuideLocked(FName GemId) const;

	bool IsGemGuideActive() const;

	static FName const GuidedGem;

	/** Saves the profile and sends PlayerClass's loadout, then refreshes the page and broadcasts OnProfileChanged. */
	void CommitChanges(EPlayerClass PlayerClass);

	/** Saves the profile and sends PlayerClass's loadout alone, for a change only the part of the page that made it
	 *  shows. */
	void CommitProfile(EPlayerClass PlayerClass);

	/** Saves the profile only (the server reads nothing of the change), then refreshes the page and broadcasts
	 *  OnProfileChanged. */
	void SaveChanges();

	/** Saves the profile alone: no send, no refresh, no broadcast. */
	void SaveProfile();

	/** A row of Class in Tint, OnClicked calling Picked; a null Picked makes a row that only reads. A null Class is the
	 * page's RowClass. */
	UGeoListRowWidget* MakeRow(EGeoListRowTint Tint, TFunction<void()> Picked,
							   TSubclassOf<UGeoListRowWidget> Class = nullptr);

	/** A text the page builds, in Role. */
	UTextBlock* MakeText(EGeoTextRole Role, FText const& Text) const;

	/** GemId's glyph at GlyphSize, in its stat colour. */
	UGeoGemGlyph* MakeGemGlyph(UGeoGemCatalog const& Catalog, FName GemId, float GlyphSize) const;

	/** Tier's name: CHIP, or CHIPS when bPlural. */
	FText GetTierName(EGeoGemTier Tier, bool bPlural) const;

	/** The gem rows, tabs and lines of the page: WBP_ListRow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TSubclassOf<UGeoListRowWidget> RowClass;

	/** The tabs of a tier strip, filtering a list: WBP_TabUnderline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TSubclassOf<UGeoListRowWidget> TabRowClass;

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
