// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/AbilityInfo.h"
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Tool/GeoColor.h"

#include "GameDataSettings.generated.h"

class AGeoEffectZone;
class UStatusInfo;
class UUserWidget;
class UWidgetComponent;
class UGameplayEffect;
class USoundBase;
class UNiagaraSystem;
class UPlayerClassDataAsset;

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
	 * Synchronously loads and returns the data asset pointed to by SoftObject, and keeps it resident for the rest of
	 * the process.
	 * @warning Should only be called after the asset has been async-loaded; synchronous loads during gameplay cause
	 * hitches.
	 */
	template <typename T>
	static T* GetLoadedDataAsset(TSoftObjectPtr<T> const& SoftObject);
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", AdvancedDisplay)
	TSoftObjectPtr<UAbilityInfo> AbilityInfo;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", AdvancedDisplay)
	TSoftObjectPtr<UPlayerClassDataAsset> PlayerClassData;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UUserWidget> DefaultDeployableHealthBarWidgetClass;

	/** Combatant health-bar WidgetComponent class (UGeoCombattantWidgetComp). Soft so gameplay never names the UI type.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UWidgetComponent> CombattantWidgetComponentClass;

	/** Default health-bar WBP (UGenericCombattantWidget subclass) for player/enemy characters. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UUserWidget> DefaultCharacterHealthBarWidgetClass;

	/** Crosshair WBP a gamepad player gets in front of their character, in place of the mouse cursor. Keep it the same
	 * asset as the Crosshairs software cursor (Project Settings -> User Interface) so both devices aim with one visual.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UUserWidget> AimCursorWidgetClass;

	/** Default click sound for UGeoButton, used when a button's own style doesn't set PressedSlateSound. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftObjectPtr<USoundBase> DefaultButtonClickSound;

	/** Default hover sound for UGeoButton, used when a button's own style doesn't set HoveredSlateSound. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSoftObjectPtr<USoundBase> DefaultButtonHoverSound;

	/** Color every FGeoColorParam of the game resolves its slot through; EGeoColor::Override is never looked up here.
	 * Materials read the same values through the palette texture AGeoGameCamera builds from this map. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Colors")
	TMap<EGeoColor, FLinearColor> ColorPalette;

	/** Zone every ability that leaves one behind spawns, unless it names a class of its own. One Blueprint for the
	 * whole game: what a zone does and the colour it draws in both come from the ability, through
	 * FDeployableDataParams. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<AGeoEffectZone> DefaultZoneClass;

	/** Closest AGeoGameCamera may zoom in — the floor for the wheel and for an AGeoCameraVolume's OrthoWidth. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "1.0"))
	float MinOrthoWidth = 2000.f;

	/** Farthest AGeoGameCamera may zoom out — the wheel's ceiling, and the width the couch-coop spread widens to
	 * once the farthest player reaches ZoomMaxDistance. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "1.0"))
	float MaxOrthoWidth = 6000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float GeneralChargeTime = .5f;

	/** Range of every player beam / line ability, and the distance span a projectile fired by a Player-team instigator
	 * resolves UseGameDataSettings to. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float GeneralSpellDistance = 1500.f;

	/** Distance span an Enemy- or Neutral-team instigator's projectile resolves UseGameDataSettings to: a boss shoots
	 * across its whole arena, far past what a player standing inside it needs. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float EnemySpellDistance = 2500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float GeneralSpellSpeed = 550.f;

	/** Project-wide default projectile radius, used when a spawn's FExternalProjectileParams leaves OverrideRadius on
	 * UseGameDataSettings. Drives both the Niagara bullet visual and the sphere collider. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Projectile")
	float GeneralProjectileRadius = 30.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Projectile")
	float MinDeployDistance = 150.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Projectile")
	float MaxDeployDistance = 1500.f;

	/** Maximum time the server fast-forwards a projectile to compensate the client's reported spawn time. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Projectile", meta = (ClampMin = "0"))
	float MaxLatencyCompensation = .5f;

	/** Maximum distance a client-reported fire origin may deviate from the server's avatar before it is snapped back.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Projectile", meta = (ClampMin = "0"))
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

	/** Shared windup telegraph (Ray Zone Indicator) every beam swaps to during its wind-up: UGeoBeamVFXComponent
	 * (player channel beams) and UBeamPattern (enemy/boss beams). One project-wide asset — no per-ability/per-pattern
	 * configuration needed. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GameFeel")
	TSoftObjectPtr<UNiagaraSystem> RayIndicatorSystem;
};

template <typename T>
T* UGameDataSettings::GetLoadedDataAsset(TSoftObjectPtr<T> const& SoftObject)
{
	T* const Asset = SoftObject.LoadSynchronous();
	if (Asset)
	{
		// A soft pointer holds a path, not a reference, and RF_Standalone only survives garbage collection in the
		// editor: outside it every asset returned here is collected and synchronously reloaded on the next access,
		// leaving anything that cached a pointer into it — an ability class, its CDO — dangling.
		Asset->AddToRoot();
	}
	return Asset;
}
