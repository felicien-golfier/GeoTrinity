// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/GeoCueParam.h"
#include "AttributeSet.h"
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
 * One buff attribute and the VFX it shows while it sits above its base value on a character. Read by every
 * UGeoFXComponent, each taking its own side: a character's UGeoGameFeelComponent wears CharacterVFX, and the shots it
 * fires wear ProjectileVFX through their UGeoProjectileFXComponent. Either may be left empty to show the buff on one
 * side only.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoBuffVFXEntry
{
	GENERATED_BODY()

	/** Watched on the buffed character's ASC — above its base value is what "buffed" means here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute Attribute;

	/** Played on the buffed character itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UNiagaraSystem> CharacterVFX;

	/** Played on a projectile the buffed character fires. Only the damage and applied-heal boosts reach a shot at all,
	 * and only on a shot that carries the matching effect — a damage buff must not light up a heal shot. Set on any
	 * other attribute it stays unused; that buff shows on the character alone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UNiagaraSystem> ProjectileVFX;
};

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

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGeneral", AdvancedDisplay)
	TSoftObjectPtr<UAbilityInfo> AbilityInfo;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGeneral", AdvancedDisplay)
	TSoftObjectPtr<UPlayerClassDataAsset> PlayerClassData;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TSoftClassPtr<UUserWidget> DefaultDeployableHealthBarWidgetClass;

	/** Combatant health-bar WidgetComponent class (UGeoCombattantWidgetComp). Soft so gameplay never names the UI type.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TSoftClassPtr<UWidgetComponent> CombattantWidgetComponentClass;

	/** Default health-bar WBP (UGenericCombattantWidget subclass) for player/enemy characters. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TSoftClassPtr<UUserWidget> DefaultCharacterHealthBarWidgetClass;

	/** Crosshair WBP a gamepad player gets in front of their character, in place of the mouse cursor. Keep it the same
	 * asset as the Crosshairs software cursor (Project Settings -> User Interface) so both devices aim with one visual.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TSoftClassPtr<UUserWidget> AimCursorWidgetClass;

	/** Default click sound for UGeoButton, used when a button's own style doesn't set PressedSlateSound. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TSoftObjectPtr<USoundBase> DefaultButtonClickSound;

	/** Default hover sound for UGeoButton, used when a button's own style doesn't set HoveredSlateSound. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TSoftObjectPtr<USoundBase> DefaultButtonHoverSound;

	/** Color every FGeoColorParam of the game resolves its slot through; EGeoColor::Override is never looked up here.
	 * Materials read the same values through the palette texture AGeoGameCamera builds from this map. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoColors")
	TMap<EGeoColor, FLinearColor> ColorPalette;

	/** Zone every ability that leaves one behind spawns, unless it names a class of its own. One Blueprint for the
	 * whole game: what a zone does and the colour it draws in both come from the ability, through
	 * FDeployableDataParams. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	TSoftClassPtr<AGeoEffectZone> DefaultZoneClass;

	/** Closest AGeoGameCamera may zoom in — the floor for the wheel and for an AGeoCameraVolume's OrthoWidth. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoCamera", meta = (ClampMin = "1.0"))
	float MinOrthoWidth = 2000.f;

	/** Farthest AGeoGameCamera may zoom out — the wheel's ceiling, and the width the couch-coop spread widens to
	 * once the farthest player reaches ZoomMaxDistance. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoCamera", meta = (ClampMin = "1.0"))
	float MaxOrthoWidth = 6000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	float GeneralChargeTime = .5f;

	/** Range of every player beam / line ability, and the distance span a projectile fired by a Player-team instigator
	 * resolves UseGameDataSettings to. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	float GeneralSpellDistance = 1500.f;

	/** Distance span an Enemy- or Neutral-team instigator's projectile resolves UseGameDataSettings to: a boss shoots
	 * across its whole arena, far past what a player standing inside it needs. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	float EnemySpellDistance = 2500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	float GeneralSpellSpeed = 550.f;

	/** Project-wide default projectile radius, used when a spawn's FExternalProjectileParams leaves OverrideRadius on
	 * UseGameDataSettings. Drives both the Niagara bullet visual and the sphere collider. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay|Projectile")
	float GeneralProjectileRadius = 30.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay|Projectile")
	float MinDeployDistance = 150.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay|Projectile")
	float MaxDeployDistance = 1500.f;

	/** Maximum time the server fast-forwards a projectile to compensate the client's reported spawn time. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay|Projectile", meta = (ClampMin = "0"))
	float MaxLatencyCompensation = .5f;

	/** Maximum distance a client-reported fire origin may deviate from the server's avatar before it is snapped back.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay|Projectile", meta = (ClampMin = "0"))
	float MaxFireOriginDeviation = 300.f;

	/** Curve to remap the raw charge ratio (0-1) and influence its charge speed.*/
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	TSoftObjectPtr<UCurveFloat> GaugeChargingSpeedCurve;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	TSoftClassPtr<UGameplayEffect> HealthEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	TSoftClassPtr<UGameplayEffect> DamageEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	TSoftClassPtr<UGameplayEffect> ShieldEffect;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameplay")
	TSoftClassPtr<UGameplayEffect> LethalEffect;

	/** Maximum number of times per second the GameplayCue on HealthEffect/DamageEffect may fire when applied every tick
	 * (drain/heal). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel|GameplayCue", meta = (ClampMin = "0.1"))
	float GameplayCueRateLimitPerSecond = 3.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel")
	TSoftObjectPtr<UMaterialInterface> HitFlashMaterial;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel")
	TSoftObjectPtr<UMaterialInterface> LocalPlayerHitFlashMaterial;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel")
	float HitFlashDuration = 0.9f;

	/** Shared generic-sound cue tag, executed locally for one-off gameplay sounds (e.g. deploy stack refilled). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel|GameplayCue")
	FGameplayTag GenericGameplayCueSoundTag;

	/** Played on the deploying client when a deploy ability's charge pool refills a stack — shared by every deploy
	 * ability (GA_DeployHealingZone, GA_Square_Special_Mine, GA_LaunchTurret) instead of a per-ability property. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel|GameplayCue")
	FGeoCueParam RefillDeployableCue;

	/** Every buff the game shows, project-wide: one entry per attribute, with the systems its character and its shots
	 * wear while it is boosted. Driven by UGeoFXComponent. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel")
	TArray<FGeoBuffVFXEntry> BuffVFX;

	/** Shared windup telegraph (Ray Zone Indicator) every beam swaps to during its wind-up: UGeoBeamVFXComponent
	 * (player channel beams) and UBeamPattern (enemy/boss beams). One project-wide asset — no per-ability/per-pattern
	 * configuration needed. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GeoGameFeel")
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
