// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoDamageNumberWidget.generated.h"

/** What kind of attribute change a floating number represents. Blueprint's SetData branches on this to pick a color
 * for each case independently (e.g. shield hits need not match health damage's color). */
UENUM(BlueprintType)
enum class EGeoDamageNumberType : uint8
{
	Damage,
	Heal,
	ShieldHit
};

/**
 * Transient screen-space widget for a single floating damage/heal/shield-hit number.
 * Activated at a world position; re-projects every tick so the number tracks the spawn point as the camera moves,
 * then drifts upward with random horizontal variance and fades out over VisibleDuration before returning to the pool.
 * Blueprint implements SetData to update the displayed text and color.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Takes this widget from the pool and starts its animation at InWorldPos (with random X/Y jitter up to
	 * LocationStartDrift). Picks a random upward-biased drift direction, then makes the widget visible so NativeTick
	 * drives the fade-and-drift. Call only when IsAvailable() is true.
	 *
	 * @param Amount     Absolute health/shield delta to display.
	 * @param Type       Which kind of change this is. Forwarded to SetData in Blueprint.
	 * @param InWorldPos World-space anchor before jitter is applied.
	 */
	void Activate(float Amount, EGeoDamageNumberType Type, FVector InWorldPos);
	/** Returns true when this widget is back in the pool and ready to be activated for a new number. */
	bool IsAvailable() const { return bAvailable; }

protected:
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void SetData(float Amount, EGeoDamageNumberType Type);

	UFUNCTION(BlueprintCallable)
	void ReturnToPool();

	UPROPERTY(EditAnywhere, Category = "GeoDamageNumbers", meta = (ClampMin = "0.1"))
	float VisibleDuration = 1.0f;

	UPROPERTY(EditAnywhere, Category = "GeoDamageNumbers", meta = (ClampMin = "0.0"))
	float DriftDistance = 60.f;

	UPROPERTY(EditAnywhere, Category = "GeoDamageNumbers")
	float LocationStartDrift = 20.f;

private:
	bool ProjectToScreen(FVector2D& OutScreenPos) const;

	bool bAvailable = true;
	bool bActive = false;
	FVector WorldPos;
	FVector2D DriftOffset;
	float ElapsedTime = 0.f;
};
