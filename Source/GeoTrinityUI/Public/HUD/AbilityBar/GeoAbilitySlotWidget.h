// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoHUD.h"
#include "HUD/GeoUserWidget.h"

#include "GeoAbilitySlotWidget.generated.h"

class UGeoFrame;
class UGeoIconImage;
class UGeoMeter;
class UTextBlock;

/**
 * One slot of the bottom-center ability bar: an ability icon with a radial cooldown sweep and countdown text,
 * plus an optional remaining-deployable count badge. All live data is pulled from AGeoHUD each tick; the slot
 * holds no gameplay state. While the ability is active the sweep is pinned full (grayed-out "in use" look) until
 * the ability ends and its cooldown takes over depleting it; with no cooldown it clears the moment the ability ends.
 * A slot can represent several abilities sharing the same input (sacrifice channel/detonate): each tick it displays
 * the last entry whose ability is active or activatable, falling back to the first.
 * A passive tile (WBP_PassiveSlot, entry with bIsPassive) is the same slot without key or count text: a passive is
 * always active, so it never pins the sweep and only its cooldown fills it.
 */
UCLASS()
class GEOTRINITYUI_API UGeoAbilitySlotWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the entries (all sharing one input) and HUD, applies the first entry, and tints the frame's glow and the
	 * value gauge in the owning player's class colour. */
	void InitSlot(TArray<FGeoAbilityBarEntry> const& InEntries, AGeoHUD* InHUD);

	/** Re-queries this slot's deploy count, refreshes the badge, and returns the current charge count. */
	UFUNCTION()
	int32 RefreshDeployCount();

	/** Re-queries the live key mapped to this slot's input action and updates KeyText only when the key changed. */
	void RefreshKeyLabel();

protected:
	/** Drives the cooldown sweep, the countdown text, the value gauge and the frame's ready look each frame. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	/** Ability icon. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoIconImage> Icon;

	/** Frame around the slot: active (lit, glowing in the class colour) while the ability has something ready to use
	 * (its IsReady) and no sweep covers it. Optional. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoFrame> SlotFrame;

	/** Sweep over the icon covering the part of the cooldown still to run. Optional. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMeter> CooldownMeter;

	/** The displayed ability's class gauge rising over the icon in the class colour (AGeoHUD::GetAbilityGauge);
	 * collapsed when it feeds none. Optional. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMeter> GaugeMeter;

	/** The class gauge's number, centered over the icon; hidden while the countdown runs, collapsed without a gauge.
	 * Optional. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GaugeText;

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

private:
	/** Picks which entry to display (last active/activatable, else the first) and refreshes the visuals on change. */
	void SelectDisplayedEntry();
	/** Applies the displayed entry's icon and deploy-badge visibility, then refreshes the badge. */
	void ApplyDisplayedEntry();
	/** Shows or hides the countdown text, skipping the call when it is already in that state. */
	void SetCountdownVisible(bool bVisible);
	/** Covers Fill of the slot with the cooldown sweep, and lights the frame when the sweep is empty and the displayed
	 * ability is ready (AGeoHUD::IsAbilityReady). */
	void SetCooldownFill(float Fill);
	/** Fills GaugeMeter and writes GaugeText from the displayed ability's class gauge, or collapses both when it feeds
	 * none. GaugeText is hidden while bCountdownVisible: a running countdown wins the centre. */
	void RefreshGauge(bool bCountdownVisible);
	FGeoAbilityBarEntry const& DisplayedEntry() const { return Entries[DisplayedIndex]; }

	/** Abilities sharing this slot's input; Entries[DisplayedIndex] drives every visual. UPROPERTY so the GC keeps the
	 * entries' Icon/InputAction assets alive. */
	UPROPERTY()
	TArray<FGeoAbilityBarEntry> Entries;
	int32 DisplayedIndex = 0;

	UPROPERTY()
	TObjectPtr<AGeoHUD> HUD;

	/** Last key shown in KeyText; lets RefreshKeyLabel skip the text update when the binding is unchanged. */
	FKey CachedKey;
};
