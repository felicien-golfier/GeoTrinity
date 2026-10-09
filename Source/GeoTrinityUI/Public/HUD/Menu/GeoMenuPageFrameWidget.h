// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoMenuPageFrameWidget.generated.h"

class UGeoButton;
class UGeoMenuButton;

/**
 * Layer 2 of every menu page: the window of UGeoMenuFrameWidget (WBP_MenuFrame) with BackButton at its bottom left and
 * CloseButton, a cross, on its top-right corner — the same two buttons in the same place on every page. One asset
 * (WBP_MenuPageFrame) carries it; the page wearing it fills its NamedSlot "PageSlot" and wires both buttons
 * (UGeoMenuPageWidget).
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", UGeoButton "CloseButton" (holding the cross icon).
 */
UCLASS()
class GEOTRINITYUI_API UGeoMenuPageFrameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGeoMenuButton* GetBackButton() const
	{
		return BackButton;
	}

	UGeoButton* GetCloseButton() const
	{
		return CloseButton;
	}

	/** Hides BACK and the cross with everything framing them, each up to the corner holding it. */
	void HideButtons();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoButton> CloseButton;
};
