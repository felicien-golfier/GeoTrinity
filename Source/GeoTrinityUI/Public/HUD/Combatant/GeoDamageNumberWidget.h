// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"

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
 * The return is a world timer, not the tick: a number placed off-screen is culled and never ticks.
 * Blueprint implements SetData to update the displayed text and color.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Takes this widget from the pool and starts its animation at InWorldPos (with random X/Y jitter up to
	 * LocationStartDrift). Picks a random upward-biased drift direction, places the widget there, makes it visible so
	 * NativeTick drives the fade-and-drift, and starts the timer that returns it to the pool after VisibleDuration.
	 * Call only when IsAvailable() is true.
	 *
	 * @param Amount     Absolute health/shield delta to display.
	 * @param Type       Which kind of change this is. Forwarded to SetData in Blueprint.
	 * @param InWorldPos World-space anchor before jitter is applied.
	 */
	void Activate(float Amount, EGeoDamageNumberType Type, FVector InWorldPos);
	/** Returns true when this widget is back in the pool and ready to be activated for a new number. */
	bool IsAvailable() const;

protected:
	/** Re-projects WorldPos each frame and applies the upward drift and fade for the lifetime timer's progress. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	/** Called from Activate; Blueprint updates the displayed text and picks a color from the Type. */
	UFUNCTION(BlueprintImplementableEvent)
	void SetData(float Amount, EGeoDamageNumberType Type);

	/** Hides the widget and stops its lifetime timer so the pool can reuse it. Fired by that timer; Blueprint may call
	 * it to end a number early. */
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
	/** Places the widget at WorldPos plus Alpha of DriftOffset and fades it to 1 - Alpha. */
	void ApplyDriftAndFade(float Alpha);

	FVector WorldPos;
	FVector2D DriftOffset;
	/** Running while the number is shown; its elapsed time is the animation clock. */
	FTimerHandle LifetimeTimerHandle;
};
