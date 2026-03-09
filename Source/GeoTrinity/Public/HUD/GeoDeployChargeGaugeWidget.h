#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoDeployChargeGaugeWidget.generated.h"

class UGeoDeployAbility;

UCLASS()
class GEOTRINITY_API UGeoDeployChargeGaugeWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Deploy")
	TObjectPtr<UGeoDeployAbility> DeployAbility;
};
