// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Style/GeoShapePainter.h"
#include "Tool/GeoIcon.h"
#include "Widgets/SLeafWidget.h"

/** Leaf widget painting one UGeoIcon over its whole box, read again on every paint so an edit to the icon shows at once.
 * UGeoIconImage wraps it for UMG; the editor's icon preview and thumbnail draw it directly. */
class SGeoIconImage : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoIconImage) {}
	SLATE_END_ARGS()

	void Construct(FArguments const& /*InArgs*/) {}

	void SetIcon(UGeoIcon const* InIcon, float const InSize, FLinearColor const& InTint)
	{
		Icon = InIcon;
		Size = InSize;
		Tint = InTint;
		Invalidate(EInvalidateWidgetReason::Layout);
	}

	virtual FVector2D ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const override
	{
		return FVector2D(Size, Size);
	}

	virtual int32 OnPaint(FPaintArgs const& /*Args*/, FGeometry const& AllottedGeometry,
						  FSlateRect const& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements, int32 LayerId,
						  FWidgetStyle const& InWidgetStyle, bool /*bParentEnabled*/) const override
	{
		if (UGeoIcon const* IconToDraw = Icon.Get())
		{
			FGeoShapePainter::DrawIcon(OutDrawElements, LayerId, AllottedGeometry, *IconToDraw,
									   FSlateRect(FVector2f::ZeroVector, AllottedGeometry.GetLocalSize()),
									   Tint * InWidgetStyle.GetColorAndOpacityTint());
		}
		return LayerId;
	}

private:
	TWeakObjectPtr<UGeoIcon const> Icon;
	float Size = 44.f;
	FLinearColor Tint = FLinearColor::White;
};
