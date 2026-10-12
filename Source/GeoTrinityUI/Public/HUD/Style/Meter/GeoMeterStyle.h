// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GeoMeterStyle.generated.h"

/** How a UGeoMeter draws its ratio. */
UENUM(BlueprintType)
enum class EGeoMeterShape : uint8
{
	/** A horizontal bar filling left to right. */
	Bar,
	/** A ring — or the frame of a polygon the meter is given — filling clockwise from its first corner. */
	Ring,
	/** A pie over the whole box covering the last Fill of a turn: a cooldown sweep. */
	Sweep
};

/**
 * The look of a UGeoMeter: a track, a fill, an overhang (the shield, drawn behind the fill and around it), tick marks
 * and an outline. One asset per kind of meter (player health, team health, boss health, class ring, cooldown, status
 * time); every meter wearing it changes with it.
 */
UCLASS(BlueprintType)
class GEOTRINITYUI_API UGeoMeterStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter")
	EGeoMeterShape Shape = EGeoMeterShape::Bar;

	/** Size asked of the layout: a bar's width and height, a ring's or sweep's box. A bar in a filling slot stretches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter")
	FVector2D Size = FVector2D(200.f, 8.f);

	/** Line width of a ring. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter", meta = (ClampMin = "0.5", ClampMax = "64"))
	float RingThickness = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Track")
	FLinearColor TrackColor = FLinearColor(1.f, 1.f, 1.f, .08f);

	/** Track colour once the fill drops under LowThreshold: a warning on a low health bar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Track")
	FLinearColor LowTrackColor = FLinearColor(1.f, 1.f, 1.f, .08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Track", meta = (ClampMin = "0", ClampMax = "1"))
	float LowThreshold = 0.f;

	/** Multiplied by the meter's FillTint, so one style fills in each class's colour. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Fill")
	FLinearColor FillColor = FLinearColor::White;

	/** Soft glow around the fill, in the fill's colour at GlowOpacity; 0 draws none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Fill", meta = (ClampMin = "0", ClampMax = "32"))
	float GlowThickness = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Fill", meta = (ClampMin = "0", ClampMax = "1"))
	float GlowOpacity = .6f;

	/** How far a bar's overhang reaches past the track on every side but the right one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Overhang", meta = (ClampMin = "0", ClampMax = "32"))
	float OverhangSize = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Overhang")
	FLinearColor OverhangFillColor = FLinearColor(.1f, .58f, 1.f, .55f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Overhang")
	FLinearColor OverhangLineColor = FLinearColor(.1f, .58f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Overhang", meta = (ClampMin = "0", ClampMax = "8"))
	float OverhangLineThickness = 1.f;

	/** Parts a bar's ticks cut it into: 4 marks the quarters, below 2 draws none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ticks", meta = (ClampMin = "0", ClampMax = "20"))
	int32 TickParts = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ticks")
	FLinearColor TickColor = FLinearColor(0.f, 0.f, 0.f, .6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ticks", meta = (ClampMin = "0.5", ClampMax = "8"))
	float TickWidth = 1.f;

	/** How far a tick sticks out above and below the bar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ticks", meta = (ClampMin = "0", ClampMax = "16"))
	float TickOverhang = 0.f;

	/** Line drawn round a bar's track, over the fill; 0 thickness draws none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Outline")
	FLinearColor OutlineColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Outline", meta = (ClampMin = "0", ClampMax = "8"))
	float OutlineThickness = 0.f;

	/** Glow under the outline, multiplied by the meter's FillTint; transparent draws none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Outline")
	FLinearColor OutlineGlowColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Outline", meta = (ClampMin = "0", ClampMax = "32"))
	float OutlineGlowThickness = 12.f;

	/** One pulse of a ready ring: it swells by ReadyPulseScale and its glow brightens, then settles back. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready", meta = (ClampMin = "0.05", ClampMax = "10"))
	float ReadyPulseSeconds = .9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready", meta = (ClampMin = "0", ClampMax = "1"))
	float ReadyPulseScale = .08f;

	/** Glow of a ready ring at the top of its pulse, multiplied by the meter's FillTint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready")
	FLinearColor ReadyGlowColor = FLinearColor(1.f, 1.f, 1.f, .8f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready", meta = (ClampMin = "0", ClampMax = "64"))
	float ReadyGlowThickness = 16.f;

	/** Small copies of the ring's shape riding round it, spinning, while it is ready. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready", meta = (ClampMin = "0", ClampMax = "16"))
	int32 ReadyOrbiterCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready", meta = (ClampMin = "1", ClampMax = "64"))
	float ReadyOrbiterSize = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready")
	FLinearColor ReadyOrbiterColor = FLinearColor::White;

	/** Time an orbiter takes to go once round the ring. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready", meta = (ClampMin = "0.1", ClampMax = "60"))
	float ReadyOrbitSeconds = 3.f;

	/** Each orbiter's own turn, in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMeter|Ready", meta = (ClampMin = "-1440", ClampMax = "1440"))
	float ReadyOrbiterSpin = 180.f;
};
