// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/AbilityInfo.h"
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "GameDataSettings.generated.h"

class UStatusInfo;
class UUserWidget;
class UGameplayEffect;

/**
 * Project Settings panel (Game Data Settings) that holds soft references to all global data assets
 * (ability info, status info, GE classes, hit flash materials, tuning values).
 * Accessible from any machine via GetDefault<UGameDataSettings>().
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Game Data Settings"))
class GEOTRINITY_API UGameDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Synchronously loads and returns the data asset pointed to by SoftObject.
	 * @warning Should only be called after the asset has been async-loaded; synchronous loads during gameplay cause hitches.
	 */
	template <typename T>
	static T* GetLoadedDataAsset(TSoftObjectPtr<T> const& SoftObject);

	/* Soft path will be converted to content reference before use */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", AdvancedDisplay)
	TSoftObjectPtr<UStatusInfo> StatusInfo;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", AdvancedDisplay)
	TSoftObjectPtr<UAbilityInfo> AbilityInfo;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UUserWidget> DefaultDeployableHealthBarWidgetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float DeployMaxChargeTime = 1.f;

	/** Curve to remap the raw charge ratio (0-1) and influence its charge speed.*/
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftObjectPtr<UCurveFloat> GaugeChargingSpeedCurve;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<UGameplayEffect> HealthEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<UGameplayEffect> DamageEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<UGameplayEffect> ShieldEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float RegularTickInterval = .1f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GameFeel")
	TSoftObjectPtr<UMaterialInterface> HitFlashMaterial;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GameFeel")
	TSoftObjectPtr<UMaterialInterface> LocalPlayerHitFlashMaterial;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GameFeel")
	float HitFlashDuration = 0.9f;
};

template <typename T>
T* UGameDataSettings::GetLoadedDataAsset(TSoftObjectPtr<T> const& SoftObject)
{
	return SoftObject.LoadSynchronous();
}
