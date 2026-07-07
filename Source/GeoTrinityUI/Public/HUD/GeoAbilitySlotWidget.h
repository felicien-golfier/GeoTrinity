// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoHUD.h"
#include "HUD/GeoUserWidget.h"

#include "GeoAbilitySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

/**
 * One slot of the bottom-center ability bar: an ability icon with a radial cooldown sweep and countdown text,
 * plus an optional remaining-deployable count badge. All live data is pulled from AGeoHUD each tick; the slot
 * holds no gameplay state. While the ability is active the sweep is pinned full (grayed-out "in use" look) until
 * the ability ends and its cooldown takes over depleting it; with no cooldown it clears the moment the ability ends.
 * A slot can represent several abilities sharing the same input (sacrifice channel/detonate): each tick it displays
 * the last entry whose ability is active or activatable, falling back to the first.
 */
UCLASS()
class GEOTRINITYUI_API UGeoAbilitySlotWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the entries (all sharing one input) and HUD, applies the first entry, and creates the cooldown-sweep
	 * material instance. */
	void InitSlot(TArray<FGeoAbilityBarEntry> const& InEntries, AGeoHUD* InHUD);

	/** Re-queries this slot's deploy count and refreshes the badge. Bound to AGeoHUD::OnPlayerDeployCountChanged. */
	UFUNCTION()
	void RefreshDeployCount();

	/** Re-queries the live key mapped to this slot's input action and updates KeyText only when the key changed. */
	void RefreshKeyLabel();

protected:
	/** Drives the cooldown-sweep material's Fill scalar and countdown text each frame; early-outs when the ability is
	 * ready. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	/** Ability icon. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Icon;

	/** Radial sweep overlaid on the icon; its material's Fill scalar is driven by cooldown progress. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> CooldownSweep;

	/** Cooldown seconds remaining, centered over the icon. Hidden when the ability is ready. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	/** Remaining-deployable count, shown only for deployable abilities. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	/** Live key binding (e.g. "LMB", "RMB", "Shift"), shown just under the slot. Refreshed each tick so rebinds appear
	 * immediately. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyText;

	/** Material on M_CooldownSweep whose Fill scalar (0=ready, 1=fully swept) the sweep image uses. */
	UPROPERTY(EditDefaultsOnly, Category = "AbilityBar")
	TObjectPtr<UMaterialInterface> CooldownSweepMaterial;

private:
	/** Picks which entry to display (last active/activatable, else the first) and refreshes the visuals on change. */
	void SelectDisplayedEntry();
	FGeoAbilityBarEntry const& DisplayedEntry() const { return Entries[DisplayedIndex]; }

	/** Abilities sharing this slot's input; Entries[DisplayedIndex] drives every visual. UPROPERTY so the GC keeps the
	 * entries' Icon/InputAction assets alive. */
	UPROPERTY()
	TArray<FGeoAbilityBarEntry> Entries;
	int32 DisplayedIndex = 0;

	UPROPERTY()
	TObjectPtr<AGeoHUD> HUD;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CooldownSweepMID;

	/** Last key shown in KeyText; lets RefreshKeyLabel skip the text update when the binding is unchanged. */
	FKey CachedKey;
};
