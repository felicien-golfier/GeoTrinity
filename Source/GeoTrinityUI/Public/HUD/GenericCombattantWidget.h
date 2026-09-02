// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GeoUserWidget.h"

#include "GenericCombattantWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * Reusable health-bar widget for any combatant (enemy, boss bar, deployable).
 * Bind it to an ASC via InitializeWithAbilitySystemComponent; attribute changes automatically update the bar.
 * Do NOT use this for the player's main overlay — use UGeoUserWidget directly for that.
 */
UCLASS()
class GEOTRINITYUI_API UGenericCombattantWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Stores ASC, calls BindStatCallbacks, then fires RefreshStats so BP subclasses can populate initial values. */
	UFUNCTION(BlueprintNativeEvent)
	void InitializeWithAbilitySystemComponent(UAbilitySystemComponent* ASC);

protected:
	UFUNCTION(BlueprintNativeEvent)
	void UpdateHealthRatio(float NewHealthRatio);
	virtual void UpdateHealthRatio_Implementation(float NewHealthRatio);

	/** Updates the ShieldBar fill to NewShieldRatio (Shield / MaxHealth). */
	UFUNCTION(BlueprintNativeEvent)
	void UpdateShieldRatio(float NewShieldRatio);
	virtual void UpdateShieldRatio_Implementation(float NewShieldRatio);

	UFUNCTION(BlueprintNativeEvent)
	void UpdateHealthBarVisibility();
	virtual void UpdateHealthBarVisibility_Implementation();

	/** Pushes the current ASC values into the bar: health ratio, shield ratio, and the bar visibility — which depends
	 * on MaxHealth, so it is only correct once the owner's attributes are initialized. Every attribute the bar shows
	 * calls this, which is what recovers the listen-server host, where MaxHealth is set synchronously before the
	 * widget's first refresh. */
	virtual void RefreshStats();

	UPROPERTY(BlueprintReadOnly, Category = "GeoRuntime")
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ShieldBar;

	/** Current health value (no max), centered over the bar. Optional — bars without it just skip the number. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentHealthText;

private:
	/** Binds every attribute the bar shows to RefreshStats, with weak lambdas that need no matching removal. */
	void BindStatCallbacks();
	/** Recomputes the shield bar as Shield / MaxHealth (shield is capped at MaxHealth by design — no MaxShield). */
	void RefreshShield();
};
