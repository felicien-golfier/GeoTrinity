// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GeoUserWidget.h"

#include "GenericCombattantWidget.generated.h"

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
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateHealthRatio(float NewHealthRatio);

	virtual void InitStats();

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

private:
	void BindStatCallbacks();
	UFUNCTION()
	void OnHealthChanged(float NewValue);
};
