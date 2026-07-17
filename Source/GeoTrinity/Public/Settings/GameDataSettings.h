// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/AbilityInfo.h"
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "GameDataSettings.generated.h"

class UStatusInfo;
class UUserWidget;
class UWidgetComponent;
class UGameplayEffect;
class USoundBase;

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
	 * @warning Should only be called after the asset has been async-loaded; synchronous loads during gameplay cause
	 * hitches.
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

	/** Combatant health-bar WidgetComponent class (UGeoCombattantWidgetComp). Soft so gameplay never names the UI type. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UWidgetComponent> CombattantWidgetComponentClass;

	/** Default health-bar WBP (UGenericCombattantWidget subclass) for player/enemy characters. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UUserWidget> DefaultCharacterHealthBarWidgetClass;

	/** Default click sound for UGeoButton, used when a button's own style doesn't set PressedSlateSound. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftObjectPtr<USoundBase> DefaultButtonClickSound;

	/** Default hover sound for UGeoButton, used when a button's own style doesn't set HoveredSlateSound. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftObjectPtr<USoundBase> DefaultButtonHoverSound;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float GeneralChargeTime = .5f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float GeneralSpellDistance = 1500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float GeneralSpellSpeed = 550.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float MinDeployDistance = 150.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float MaxDeployDistance = 1500.f;

	/** Maximum time the server fast-forwards a projectile to compensate the client's reported spawn time. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0"))
	float MaxLatencyCompensation = .5f;

	/** Maximum distance a client-reported fire origin may deviate from the server's avatar before it is snapped back. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0"))
	float MaxFireOriginDeviation = 300.f;

	/** Curve to remap the raw charge ratio (0-1) and influence its charge speed.*/
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftObjectPtr<UCurveFloat> GaugeChargingSpeedCurve;

	/** Shared generic-sound cue tag, executed locally for one-off gameplay sounds (e.g. deploy stack refilled). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	FGameplayTag GenericGameplayCueSoundTag;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<UGameplayEffect> HealthEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<UGameplayEffect> DamageEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<UGameplayEffect> ShieldEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<UGameplayEffect> LethalEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float RegularTickInterval = .1f;

	/** Maximum number of times per second the GameplayCue on HealthEffect/DamageEffect may fire when applied every tick
	 * (drain/heal). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0.1"))
	float GameplayCueRateLimitPerSecond = 3.f;

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
