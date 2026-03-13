// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GeoUserWidget.h"

#include "GenericCombattantWidget.generated.h"

class UProgressBar;

/**
 *
 */
UCLASS()
class GEOTRINITY_API UGenericCombattantWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void InitializeWithAbilitySystemComponent(UAbilitySystemComponent* ASC);

protected:
	UFUNCTION(BlueprintNativeEvent)
	void UpdateHealthRatio(float NewHealthRatio);
	virtual void UpdateHealthRatio_Implementation(float NewHealthRatio);

	virtual void InitStats();

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

private:
	void BindStatCallbacks();
	UFUNCTION()
	void OnHealthChanged(float NewValue);
};
