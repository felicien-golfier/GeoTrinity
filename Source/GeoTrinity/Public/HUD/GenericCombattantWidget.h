// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GeoUserWidget.h"

#include "GenericCombattantWidget.generated.h"

class UProgressBar;

/**
 * Reusable health-bar widget for any combatant (enemy, boss bar, deployable).
 * Bind it to an ASC via InitializeWithAbilitySystemComponent; attribute changes automatically update the bar.
 * Do NOT use this for the player's main overlay — use UGeoUserWidget directly for that.
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
