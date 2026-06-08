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
 * holds no gameplay state. Tick early-outs while the ability is ready and not deployable, so an idle bar is free.
 */
UCLASS()
class GEOTRINITY_API UGeoAbilitySlotWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the entry and HUD, sets the icon brush, and creates the cooldown-sweep material instance. */
	void InitSlot(FGeoAbilityBarEntry const& InEntry, AGeoHUD* InHUD);

	/** Re-queries this slot's deploy count and refreshes the badge. Bound to AGeoHUD::OnPlayerDeployCountChanged. */
	UFUNCTION()
	void RefreshDeployCount();

protected:
	/** Drives the cooldown-sweep material's Fill scalar and countdown text each frame; early-outs when the ability is ready. */
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

	/** Material on M_CooldownSweep whose Fill scalar (0=ready, 1=fully swept) the sweep image uses. */
	UPROPERTY(EditDefaultsOnly, Category = "AbilityBar")
	TObjectPtr<UMaterialInterface> CooldownSweepMaterial;

private:
	FGeoAbilityBarEntry Entry;

	UPROPERTY()
	TObjectPtr<AGeoHUD> HUD;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CooldownSweepMID;
	
	float tmp;
};
