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
class AGeoHUD;

/**
 * Row of icons showing every active effect on the local player that carries one, with a stack-count badge when the
 * same icon is active more than once, a remaining-time countdown, and a radial depletion sweep like the ability
 * slots. Built entirely in C++ (no WBP asset): the widget tree is a canvas with a horizontal box filling the slot
 * the widget gets in WBP_MainOverlay; each icon sits in a ScaleBox so it scales to fit the bar. Polls
 * AGeoHUD::GetActiveEffectIcons each tick; the row is rebuilt only when the icon set changes,
 * count/timer texts and the sweep fill are updated in place. Bound as "StatusBar" on
 * UGeoOverlayWidget (WBP_MainOverlay); InitStatusBar is called from AGeoHUD::InitOverlay via the overlay.
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
};
