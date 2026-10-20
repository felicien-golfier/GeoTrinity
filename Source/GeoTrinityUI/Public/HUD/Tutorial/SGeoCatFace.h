// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * The Cat's face, the Tutorial Board's catface / catfacet symbols: flat shapes from a 200x200 design box fitted into
 * the widget, blinking and flicking its ears on one active timer, its mouth moving while it talks. Palette roles in
 * Tutorial/Cat.md. Face only: no body, no tail.
 */
class SGeoCatFace : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoCatFace) {}
	SLATE_END_ARGS()

	void Construct(FArguments const& InArgs);

	/** Head, ears. */
	void SetBase(FLinearColor const& InBase);
	/** Inner ears. */
	void SetShade(FLinearColor const& InShade);
	/** Mouth. */
	void SetInk(FLinearColor const& InInk);
	/** Eyes, nose. */
	void SetLight(FLinearColor const& InLight);
	/** Width and height asked of the layout; the face fits whatever box it is given. */
	void SetSize(float InSize);
	/** The mouth moves while true, stays closed otherwise. */
	void SetTalking(bool bInTalking);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(FPaintArgs const& Args, FGeometry const& AllottedGeometry, FSlateRect const& MyCullingRect,
						  FSlateWindowElementList& OutDrawElements, int32 LayerId, FWidgetStyle const& InWidgetStyle,
						  bool bParentEnabled) const override;

private:
	/** Keeps the loops running: stores the time and asks for a repaint. */
	EActiveTimerReturnType Animate(double CurrentTime, float DeltaTime);

	/** Points of the design box turned by AngleDegrees and scaled vertically by ScaleY about Pivot, then fitted into
	 * the widget's box. */
	TArray<FVector2f> Place(TArray<FVector2f> const& DesignPoints, FGeometry const& Geometry,
							FVector2f Pivot = FVector2f::ZeroVector, float AngleDegrees = 0.f,
							float ScaleY = 1.f) const;

	/** Fills Rim, a closed shape that may be concave as long as it is star-shaped around its centre of corners. */
	void DrawShape(FSlateWindowElementList& OutDrawElements, int32 LayerId, FGeometry const& Geometry,
				   TArray<FVector2f> const& Rim, FLinearColor const& Color) const;

	FLinearColor Base = FLinearColor::White;
	FLinearColor Shade = FLinearColor::White;
	FLinearColor Ink = FLinearColor::Black;
	FLinearColor Light = FLinearColor::White;
	float Size = 56.f;
	bool bTalking = false;
	double Seconds = 0.;
};
