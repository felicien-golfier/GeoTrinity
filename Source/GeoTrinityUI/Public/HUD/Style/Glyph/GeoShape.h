// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "GeoShape.generated.h"

class SGeoShape;
class UGeoFrameStyle;
struct FGeoClassStyle;

/**
 * A flat regular polygon or circle, hollow or solid, optionally turning: the class shapes, the hex shells of the main
 * menu, any geometric ornament. With an OutlineStyle it wears a frame style instead — glow, line, fill and runners
 * travelling round it, as an idle frame does. Draws with Slate primitives on an active timer — no texture, no material.
 */
UCLASS()
class GEOTRINITYUI_API UGeoShape : public UWidget
{
	GENERATED_BODY()

public:
	/** Becomes the solid class shape of ClassStyle, in its colour. */
	void SetClassShape(FGeoClassStyle const& ClassStyle);

	/** Pushes Sides, Size, Rotation, SpinSpeed, Color and OutlineStyle to the underlying SGeoShape. */
	virtual void SynchronizeProperties() override;
	/** Releases the SGeoShape Slate widget. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

	/** Corner count: 3 triangle, 4 square, 6 hexagon. Below 3 draws a circle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape", meta = (ClampMin = "0", ClampMax = "24"))
	int32 Sides = 6;

	/** Width and height of the box the shape is drawn in; the shape touches its edges. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape", meta = (ClampMin = "1", ClampMax = "4096"))
	float Size = 64.f;

	/** Turn in degrees, clockwise. 0 puts a corner straight up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape")
	float Rotation = 0.f;

	/** Turn speed in degrees per second; negative turns counter-clockwise, 0 stays still. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape", meta = (ClampMin = "-720", ClampMax = "720"))
	float SpinSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape")
	bool bFilled = false;

	/** Outline thickness of a hollow shape. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape", meta = (ClampMin = "0.5", ClampMax = "32"))
	float LineThickness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape")
	FLinearColor Color = FLinearColor::White;

	/** Draws the outline in this frame style — glow, line, fill, runners — in place of Color, LineThickness, bFilled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShape")
	TObjectPtr<UGeoFrameStyle> OutlineStyle;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SGeoShape> MyShape;
};
