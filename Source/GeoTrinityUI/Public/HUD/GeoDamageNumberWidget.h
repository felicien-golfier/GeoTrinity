// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoDamageNumberWidget.generated.h"

/**
 * Transient screen-space widget for a single floating damage or heal number.
 * Position is set via SetRenderTranslation at activation time.
 * Blueprint implements SetData to run the pop/float/fade animation, then calls ReturnToPool when done.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Activate(float Amount, bool bIsHeal, FVector2D ScreenPos);
	bool IsAvailable() const { return bAvailable; }

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SetData(float Amount, bool bIsHeal);

	UFUNCTION(BlueprintCallable)
	void ReturnToPool();

private:
	bool bAvailable = true;
};
