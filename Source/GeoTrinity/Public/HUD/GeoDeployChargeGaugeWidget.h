// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoDeployChargeGaugeWidget.generated.h"


class UGeoGameplayAbility;
class UProgressBar;

UCLASS()
class GEOTRINITY_API UGeoDeployChargeGaugeWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Deploy")
	TObjectPtr<UGeoGameplayAbility> DeployAbility;

protected:
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ChargeBar;
};
