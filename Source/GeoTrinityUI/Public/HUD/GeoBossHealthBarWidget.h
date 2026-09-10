// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GenericCombattantWidget.h"

#include "GeoBossHealthBarWidget.generated.h"

class AGeoArena;
class UTextBlock;

/**
 * The bar AGeoHUD puts on screen for a boss fight: a UGenericCombattantWidget with the fight timer beside it. The
 * timer reads the live arena's own elapsed time, counted on the local clock from a start every machine derives from
 * the same replicated one — and it is on screen exactly while a boss is, since the HUD creates this widget when a
 * fight starts and drops it when it ends. The final time therefore lives on past the bar only on the combat-stats
 * panel, which shows the same value and keeps it.
 * Required in the BP hierarchy for the timer to show: a UTextBlock named "FightTimerText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoBossHealthBarWidget : public UGenericCombattantWidget
{
	GENERATED_BODY()

protected:
	/** Writes the fight's elapsed time into FightTimerText. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	/** Time since the fight started, beside the bar. Optional: a bar authored without one simply shows no timer. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FightTimerText;

private:
	/** The arena being fought, resolved on the first tick — this bar only lives while exactly one fight is running. */
	TWeakObjectPtr<AGeoArena> Arena;
};
