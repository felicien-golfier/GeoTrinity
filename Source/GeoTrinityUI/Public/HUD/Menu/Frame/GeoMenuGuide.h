// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"
#include "Layout/SlateRect.h"

/** What a menu guide points at: the control to click next and the short text shown by it. No widget: nothing to point
 * at (the hint then stands alone). AlsoLit, when set, is a second control the hole also covers: the hole is the
 * rectangle holding both. */
struct FGeoMenuGuideTarget
{
	FGeoMenuGuideTarget() = default;
	FGeoMenuGuideTarget(UWidget* InWidget, FText InHint, UWidget* InAlsoLit = nullptr) :
		Widget(InWidget), Hint(MoveTemp(InHint)), AlsoLit(InAlsoLit)
	{
	}

	TWeakObjectPtr<UWidget> Widget;
	FText Hint;
	TWeakObjectPtr<UWidget> AlsoLit;
};

/** Where a guide's hint label goes: its anchor in the canvas and which point of the label sits on it (0,0
 * top-left). */
struct FGeoMenuGuideHintPlace
{
	FVector2f Position = FVector2f::ZeroVector;
	FVector2f Alignment = FVector2f::ZeroVector;
};

namespace GeoMenuGuide
{
	/** The rectangles that dim Canvas but Hole (clamped to Canvas): above, below, left and right of it, those with
	 * an area only. The hole itself holds no rectangle, so a click there reaches the real control. */
	GEOTRINITYUI_API TArray<FSlateRect> MakeScrim(FSlateRect const& Canvas, FSlateRect const& Hole);

	/** Hint label spot: above Hole when RoomNeeded fits over it in Canvas, else below; on the hole's left edge, or
	 * its right edge (label growing leftwards) when the hole sits in the right half of Canvas. Gap separates label
	 * and hole. */
	GEOTRINITYUI_API FGeoMenuGuideHintPlace PlaceHint(FSlateRect const& Canvas, FSlateRect const& Hole,
													  float RoomNeeded, float Gap);
} // namespace GeoMenuGuide
