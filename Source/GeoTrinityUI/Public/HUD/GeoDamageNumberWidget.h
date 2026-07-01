// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoDamageNumberWidget.generated.h"

/**
 * Transient screen-space widget for a single floating damage or heal number.
 * Activated at a world position; re-projects every tick so the number tracks the spawn point as the camera moves,
 * then drifts upward with random horizontal variance and fades out over VisibleDuration before returning to the pool.
 * Blueprint implements SetData to update the displayed text and color.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Activate(float Amount, bool bIsHeal, FVector InWorldPos);
	bool IsAvailable() const { return bAvailable; }

protected:
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void SetData(float Amount, bool bIsHeal);

	UFUNCTION(BlueprintCallable)
	void ReturnToPool();

	UPROPERTY(EditAnywhere, Category = "DamageNumbers", meta = (ClampMin = "0.1"))
	float VisibleDuration = 1.0f;

	UPROPERTY(EditAnywhere, Category = "DamageNumbers", meta = (ClampMin = "0.0"))
	float DriftDistance = 60.f;

	UPROPERTY(EditAnywhere, Category = "DamageNumbers")
	float LocationStartDrift = 20.f;

private:
	bool ProjectToScreen(FVector2D& OutScreenPos) const;

	bool bAvailable = true;
	bool bActive = false;
	FVector WorldPos;
	FVector2D DriftOffset;
	float ElapsedTime = 0.f;
};
