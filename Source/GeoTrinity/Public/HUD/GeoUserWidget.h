// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GeoUserWidget.generated.h"

class AGeoHUD;
/**
 * A derived class to allow easy binding of stuff
 */
UCLASS()
class GEOTRINITY_API UGeoUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	void InitFromHUD(AGeoHUD* GeoHUD);
	
	UFUNCTION(BlueprintImplementableEvent)
	void BindCallbacksFromHUD(AGeoHUD* GeoHUD);
};
