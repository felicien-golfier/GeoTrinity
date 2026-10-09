// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoListPanelWidget.generated.h"

class UGeoListFrameWidget;
class UGeoListRowWidget;

/**
 * Base of every full-screen list page. The page wears a UGeoListFrameWidget (WBP_ListPanel) inside its page frame
 * and fills it with rows built from RowWidgetClass (WBP_ListRow), so the server browser and the leaderboard are one
 * list with different columns, skinned by those two assets alone. A subclass builds its own controls into the list
 * frame's header and footer named slots in its BP tree, and its rows from MakeRow.
 * Required in the BP hierarchy: UGeoListFrameWidget "ListFrame".
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoListPanelWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

public:
	/** Row Blueprint every list in the game is built from, so all of them are skinned in one place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoList")
	TSubclassOf<UGeoListRowWidget> RowWidgetClass;

protected:
	/** A row of RowWidgetClass, for the caller to fill with columns and add to a box. Null when unconfigured. */
	UGeoListRowWidget* MakeRow();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoListFrameWidget> ListFrame;
};
