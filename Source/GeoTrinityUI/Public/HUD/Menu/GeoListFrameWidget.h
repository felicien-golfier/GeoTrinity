// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoListFrameWidget.generated.h"

class UScrollBox;

/**
 * The frame every full-screen list wears: the header strip the panel drops its own controls into, the framed area
 * its rows scroll in, and the bottom-right corner its back button sits in. One asset (WBP_ListPanel) carries that
 * look and every list panel instantiates it, so editing it re-skins the server browser and the leaderboard at once
 * — the same deal WBP_ListRow strikes for the rows inside.
 * HeaderSlot and FooterSlot are filled by the panel owning the frame and never touched from here, so they stay BP
 * variables; only the box the rows go in is reached from code.
 * Required in the BP hierarchy: UScrollBox "RowsBox", UNamedSlot "HeaderSlot", "FooterSlot".
 */
UCLASS()
class GEOTRINITYUI_API UGeoListFrameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UScrollBox> RowsBox;
};
