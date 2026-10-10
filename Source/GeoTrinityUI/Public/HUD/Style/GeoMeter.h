// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "GeoMeter.generated.h"

class SGeoMeter;
class UGeoMeterStyle;

/**
 * A ratio drawn in a UGeoMeterStyle: a health bar with its shield overhang, a class gauge frame, a cooldown sweep, a
 * status timer. A ring can take a polygon's shape and, while ready, pulses with small copies of that shape riding round
 * it. Draws with Slate primitives: no texture, no material.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMeter : public UWidget
{
	GENERATED_BODY()

public:
	/** Sets the fill ratio, where 0 is empty and 1 is full. */
	UFUNCTION(BlueprintCallable, Category = "GeoMeter")
	void SetFill(float InFill);

	/** Sets the overhang ratio drawn behind the fill — the shield over max health. Range: [0, 1]. */
	UFUNCTION(BlueprintCallable, Category = "GeoMeter")
	void SetOverhang(float InOverhang);

	/** Multiplies the style's fill and outline glow colours, so one style fills in each class's colour. */
	UFUNCTION(BlueprintCallable, Category = "GeoMeter")
	void SetFillTint(FLinearColor const& InFillTint);

	/** Switches to InMeterStyle; null falls back to no style. */
	UFUNCTION(BlueprintCallable, Category = "GeoMeter")
	void SetMeterStyle(UGeoMeterStyle* InMeterStyle);

	/** Gives a ring InSides corners (below 3 a circle) turned InRotation degrees: the frame of a class shape. */
	UFUNCTION(BlueprintCallable, Category = "GeoMeter")
	void SetRingShape(int32 InSides, float InRotation);

	/** Activates the ready look: the ring pulses and its orbiters travel around it, as the style's Ready values say. */
	UFUNCTION(BlueprintCallable, Category = "GeoMeter")
	void SetReady(bool bInReady);

	/** Pushes MeterStyle, Fill, Overhang, FillTint, RingSides, RingRotation and bReady to the underlying SGeoMeter. */
	virtual void SynchronizeProperties() override;
	/** Releases the SGeoMeter Slate widget. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter")
	TObjectPtr<UGeoMeterStyle> MeterStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter", meta = (ClampMin = "0", ClampMax = "1"))
	float Fill = 1.f;

	/** Ratio of the overhang drawn behind the fill: the shield over max health. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter", meta = (ClampMin = "0", ClampMax = "1"))
	float Overhang = 0.f;

	/** Multiplies the style's fill and outline glow colours, so one style fills in each class's colour. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter")
	FLinearColor FillTint = FLinearColor::White;

	/** Corners of a ring: below 3 a circle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter", meta = (ClampMin = "0", ClampMax = "24"))
	int32 RingSides = 0;

	/** Turn of a ring's polygon in degrees, clockwise; 0 puts a corner straight up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter")
	float RingRotation = 0.f;

	/** A ring pulses and sends its orbiters round while ready, as its style's Ready values say. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter")
	bool bReady = false;

private:
	TSharedPtr<SGeoMeter> MyMeter;
};
