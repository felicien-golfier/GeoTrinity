// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoListPanelWidget.generated.h"

class UGeoListFrameWidget;
class UGeoListRowWidget;
class UGeoMenuButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoListPanelClosedSignature);

/**
 * Base of every full-screen list panel. The panel carries no look of its own: it wears a UGeoListFrameWidget
 * (WBP_ListPanel) as its whole tree and fills it with rows built from RowWidgetClass (WBP_ListRow), so the server
 * browser and the leaderboard are one list with different columns, skinned by those two assets alone. A subclass
 * builds its own controls into the frame's header and footer named slots in its BP tree, and its rows from MakeRow.
 * Communicates back to the menu that opened it exclusively via the OnClosed delegate, which fires from both
 * BackButton and the panel's back input.
 * Required in the BP hierarchy: UGeoListFrameWidget "ListFrame", UGeoMenuButton "BackButton".
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoListPanelWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GeoList")
	FGeoListPanelClosedSignature OnClosed;

	/** Row Blueprint every list in the game is built from, so all of them are skinned in one place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoList")
	TSubclassOf<UGeoListRowWidget> RowWidgetClass;

protected:
	/** Wires BackButton. */
	virtual void NativeConstruct() override;
	/** Returns BackButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Fires OnClosed and consumes the back input. */
	virtual bool HandleBackAction() override;

	/** A row of RowWidgetClass, for the caller to fill with columns and add to a box. Null when unconfigured. */
	UGeoListRowWidget* MakeRow();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoListFrameWidget> ListFrame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

private:
	UFUNCTION()
	void HandleBack();
};
