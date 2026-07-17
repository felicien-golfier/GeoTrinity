// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoStatusBarWidget.generated.h"

class UTextBlock;
class UHorizontalBox;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProgressBar;
class AGeoHUD;

/**
 * Row of icons showing every active effect on the local player that carries one, with a stack-count badge when the
 * same icon is active more than once, a remaining-time countdown, and a radial depletion sweep like the ability
 * slots. Synthetic gauge entries (FillRatio >= 0) instead reveal their icon bottom-to-top with the fill and tint it
 * FullColor when full. Built entirely in C++ (no WBP asset): the widget tree is a canvas filling the slot the widget gets in
 * WBP_MainOverlay, holding a centered horizontal box; each icon is a square sized to the bar's height, so resizing
 * the StatusBar slot in the overlay is what drives the icon size. Polls AGeoHUD::GetActiveEffectIcons each tick; the
 * row is rebuilt only when the icon set changes, count/timer texts and the sweep fill are updated in place. Bound as
 * "StatusBar" on UGeoOverlayWidget (WBP_MainOverlay); InitStatusBar is called from AGeoHUD::InitOverlay via the overlay.
 */
UCLASS()
class GEOTRINITYUI_API UGeoStatusBarWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the HUD reference the tick polls for active effect icons. Called by AGeoHUD right after CreateWidget. */
	void InitStatusBar(AGeoHUD* GeoHUD);

protected:
	/** Constructs the widget tree: a canvas panel containing an auto-sized horizontal box anchored bottom-center. */
	virtual bool Initialize() override;
	/** Polls AGeoHUD::GetActiveEffectIcons and rebuilds the icon row when the set changes; updates count and timer texts in place each tick. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	/** Material on M_CooldownSweep whose Fill scalar (0=full icon, 1=fully depleted) the depletion sweep uses. */
	UPROPERTY(EditDefaultsOnly, Category = "StatusBar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> DepletionSweepMaterial;

private:
	/** Container the status icons are added to. */
	UPROPERTY()
	TObjectPtr<UHorizontalBox> StatusBox;

	UPROPERTY()
	TObjectPtr<AGeoHUD> HUD;

	/** Icons currently displayed (textures or materials); the row is rebuilt only when this set changes. */
	UPROPERTY()
	TArray<UObject*> DisplayedIcons;

	/** Square side (bar height) last applied to the icons; icons are resized only when the bar height changes. */
	float AppliedIconSize = 0.f;

	/** Per-icon image, parallel to DisplayedIcons; square-sized to the bar height. */
	UPROPERTY()
	TArray<TObjectPtr<UImage>> IconImages;

	/** Per-icon stack-count badge, parallel to DisplayedIcons; updated in place each tick. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> CountTexts;

	/** Per-icon remaining-time countdown, parallel to DisplayedIcons; updated in place each tick. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> TimerTexts;

	/** Per-icon radial depletion overlay, parallel to DisplayedIcons; Fill scalar updated in place each tick. */
	UPROPERTY()
	TArray<TObjectPtr<UImage>> DepletionSweeps;

	/** Per-icon depletion sweep material instances, parallel to DisplayedIcons. */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DepletionSweepMIDs;

	/** Per-icon bottom-to-top gauge fill (icon revealed by a masked progress bar over a dimmed copy), parallel to
	 * DisplayedIcons; nullptr for regular effect entries, which use the depletion sweep instead. */
	UPROPERTY()
	TArray<TObjectPtr<UProgressBar>> GaugeBars;
};
