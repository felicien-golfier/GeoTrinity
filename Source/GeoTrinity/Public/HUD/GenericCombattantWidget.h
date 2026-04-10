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
	/** Stores ASC, calls BindStatCallbacks, then fires InitStats so BP subclasses can populate initial values. */
	UFUNCTION(BlueprintNativeEvent)
	void InitializeWithAbilitySystemComponent(UAbilitySystemComponent* ASC);

	/** Removes attribute change delegates bound by BindStatCallbacks. Call before the ASC is destroyed. */
	void UnbindStatCallbacks();

protected:
	UFUNCTION(BlueprintNativeEvent)
	void UpdateHealthRatio(float NewHealthRatio);
	virtual void UpdateHealthRatio_Implementation(float NewHealthRatio);

	UFUNCTION(BlueprintNativeEvent)
	void UpdateHealthBarVisibility();
	virtual void UpdateHealthBarVisibility_Implementation();

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
