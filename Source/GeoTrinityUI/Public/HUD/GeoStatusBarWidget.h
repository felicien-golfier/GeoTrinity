// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoStatusBarWidget.generated.h"

class UTextBlock;
class UHorizontalBox;
class AGeoHUD;

/**
 * Row of icons showing every active effect on the local player that carries one, with a stack-count badge when the
 * same icon is active more than once and a remaining-time countdown like the ability slots. Built entirely in C++
 * (no WBP asset): the widget tree is a canvas with a horizontal box anchored bottom-center above the ability bar.
 * Polls AGeoHUD::GetActiveEffectIcons each tick; the row is rebuilt only when the icon set changes, count and timer
 * texts are updated in place. Created and added to the viewport by AGeoHUD::InitOverlay.
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
};
