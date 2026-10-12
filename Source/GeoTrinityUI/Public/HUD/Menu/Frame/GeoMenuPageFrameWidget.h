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
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", UGeoButton "CloseButton" (holding the cross icon) and
 * "CloseBox", the widget holding CloseButton and its frame.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMenuPageFrameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Returns the Back button at the frame's bottom-left corner. */
	UGeoMenuButton* GetBackButton() const
	{
		return BackButton;
	}

	/** Returns the Close cross button at the frame's top-right corner. */
	UGeoButton* GetCloseButton() const
	{
		return CloseButton;
	}

	/** Collapses BackButton and CloseBox. */
	void HideButtons();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoButton> CloseButton;

	/** The cross's corner: CloseButton with the frame drawn around it. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> CloseBox;
};
