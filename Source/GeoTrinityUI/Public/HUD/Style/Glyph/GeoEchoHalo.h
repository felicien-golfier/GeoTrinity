// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "GeoEchoHalo.generated.h"

class SGeoEchoHalo;
class UGeoFrameStyle;

/**
 * The halo behind the echo socket: a dark disc, a ring with runner squares travelling it like a button frame's, and two
 * rosaces of outline squares, each square turned a step from the last, turning against each other. Runner size, speeds,
 * glow and the idle / active blend come from FrameStyle; the colour from the caller. Locked, it is grey, still and
 * without runners. Draws with FGeoShapePainter on an active timer and never takes input.
 */
UCLASS()
class GEOTRINITYUI_API UGeoEchoHalo : public UWidget
{
	GENERATED_BODY()

public:
	/** The colour of the rings, squares and runners while the halo is not locked. */
	void SetColor(FLinearColor const& InColor);

	/** A locked halo is dim, still and drawn in LockedColor. */
	void SetLocked(bool bInLocked);

	/** An active halo turns faster, glows brighter and runs its squares faster, as FrameStyle's active look. */
	void SetActive(bool bInActive);

	/** Holds the active look for FlareSeconds, then eases back. */
	void PlayFlare();

	/** Releases the SGeoEchoHalo Slate widget. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

	/** Runner size, speeds, glow, line colours and the idle / active blend of the halo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo")
	TObjectPtr<UGeoFrameStyle> FrameStyle;

	/** Squares in each rosace; they share a quarter turn evenly, so more of them make a rounder star. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo", meta = (ClampMin = "1", ClampMax = "32"))
	int32 SquareCount = 8;

	/** Radius of the disc and the ring, as a share of half the widget's box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo", meta = (ClampMin = "0.1", ClampMax = "1"))
	float RingRatio = .95f;

	/** Corner radius of the squares of the outer rosace, as a share of half the widget's box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo", meta = (ClampMin = "0.1", ClampMax = "1"))
	float OuterRosaceRatio = .78f;

	/** Corner radius of the squares of the inner rosace, which turns against the outer one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo", meta = (ClampMin = "0.1", ClampMax = "1"))
	float InnerRosaceRatio = .5f;

	/** Turn of the outer rosace in degrees per second while idle; the inner one turns the other way at the same speed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo", meta = (ClampMin = "0", ClampMax = "360"))
	float IdleTurnSpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo", meta = (ClampMin = "0", ClampMax = "360"))
	float ActiveTurnSpeed = 30.f;

	/** How long PlayFlare holds the active look. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo", meta = (ClampMin = "0", ClampMax = "5"))
	float FlareSeconds = .8f;

	/** Alpha of the soft light the halo's colour casts over the disc while idle, and its multiple while active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo|Light", meta = (ClampMin = "0", ClampMax = "1"))
	float BloomAlpha = .35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo|Light", meta = (ClampMin = "1", ClampMax = "4"))
	float ActiveBloomScale = 1.8f;

	/** Nested discs the bloom is built of; more is smoother. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo|Light", meta = (ClampMin = "1", ClampMax = "16"))
	int32 BloomLayers = 8;

	/** Alpha of the glow along the squares and the ring, idle and active (FrameStyle gives only its thickness). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo|Light", meta = (ClampMin = "0", ClampMax = "1"))
	float IdleGlowAlpha = .4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo|Light", meta = (ClampMin = "0", ClampMax = "1"))
	float ActiveGlowAlpha = .8f;

	/** The ring's glow is FrameStyle's glow thickness times this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo|Light", meta = (ClampMin = "1", ClampMax = "10"))
	float RingGlowScale = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo")
	FLinearColor DiscColor = FLinearColor(.0024f, .0018f, .0103f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEchoHalo")
	FLinearColor LockedColor = FLinearColor(.5f, .5f, .5f, .35f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	friend class SGeoEchoHalo;

	FLinearColor Color = FLinearColor::White;
	bool bLocked = false;
	bool bActive = false;
	/** Set by PlayFlare, taken by the Slate widget on its next step. */
	bool bFlareRequested = false;

	TSharedPtr<SGeoEchoHalo> MyHalo;
};
