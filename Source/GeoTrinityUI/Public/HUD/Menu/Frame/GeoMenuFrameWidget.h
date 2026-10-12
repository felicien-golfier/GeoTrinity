// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoMenuFrameWidget.generated.h"

/**
 * Layer 1 of every menu page: the bare window — the whole screen inset by the theme's MenuFrameMargin, the same for
 * every page, never sized by what it holds. One asset (WBP_MenuFrame) carries its look; a page wanting the window
 * without Back and Close wears it directly, any other page wears UGeoMenuPageFrameWidget, which wears this.
 * Its NamedSlot "ContentSlot" takes the content inside the frame's padding, "CornerSlot" a widget on its top-right
 * corner, over the padding (the close cross), "BackCornerSlot" one on its bottom-left corner (BACK).
 * Required in the BP hierarchy: "FrameBox", the window, in an Overlay.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMenuFrameWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Insets FrameBox by the theme's margin, so the designer previews the size the game shows. */
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> FrameBox;
};
