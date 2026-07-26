// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/GeoSoundRow.h"
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Tool/GeoColor.h"
#include "Tool/Team.h"

#include "ExternalProjectileParams.generated.h"

class AGeoProjectile;

UENUM(BlueprintType)
enum class EProjectileSoundType : uint8
{
	Start,
	Looping,
	/** Played when the projectile's life ends without a valid target: wall hit, distance span, or lifespan. */
	NoOverlapEnd,
	/** Played when the projectile overlaps a valid target. */
	ValidOverlapEnd,
};

/**
 * How a single projectile spawn param resolves its value:
 * - UseGameDataSettings: read the project-wide value from UGameDataSettings. Only distance, speed and radius have such
 *   a setting; for every other param this behaves exactly like KeepBlueprintDefaultValue.
 * - KeepBlueprintDefaultValue: use the projectile Blueprint's own DefaultParams value.
 * - OverrideValue: use the explicit value stored in the same struct.
 */
UENUM(BlueprintType)
enum class EOverrideParam : uint8
{
	UseGameDataSettings UMETA(DisplayName = "Use Game Data Settings"),
	KeepBlueprintDefaultValue UMETA(DisplayName = "Keep Blueprint Default"),
	OverrideValue UMETA(DisplayName = "Override")
};

/**
 * Every value a projectile runs with: travel distance and speed, radius (drives both the sphere collider and the
 * "User.Bullet_Radius" param), head/trail color, trail lifetime, the sound map, the overlap team attitude, and the
 * self-overlap rule. Used as a projectile's per-Blueprint defaults (AGeoProjectile::DefaultParams, edited in Class
 * Defaults), as its resolved runtime bundle (AGeoProjectile::ResolvedParams), and as the override values of
 * FExternalProjectileParams, which derives from this so all three share one declaration.
 *
 * Each value carries an OverrideToggle metadata naming the EOverrideParam it belongs to. It has no effect here (a
 * projectile's own defaults always show); FExternalProjectileParamsCustomization reads it to hide the value in a spawn
 * site's params whenever that toggle is not on OverrideValue. This is why the toggles cannot be reached through a plain
 * EditCondition: the engine resolves an EditCondition operand only against the struct that *declares* the conditioned
 * property, so an inherited value can never see a toggle added by the derived struct.
 */
USTRUCT(BlueprintType)
struct FProjectileParamsBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (ClampMin = "0", UIMin = "0", OverrideToggle = "OverrideDistanceSpan"))
	float DistanceSpan = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0", OverrideToggle = "OverrideSpeed"))
	float ProjectileSpeed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0", OverrideToggle = "OverrideRadius"))
	float Radius = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (OverrideToggle = "OverrideHeadColor"))
	FGeoColorParam HeadColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (OverrideToggle = "OverrideTrailColor"))
	FGeoColorParam TrailColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (ClampMin = "0", UIMin = "0", OverrideToggle = "OverrideTrailLifetimeScale"))
	float TrailLifetimeScale = 1.f;

	/** Per-sound-type sound asset + volume + audience + attribute-driven pitch mapping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (OverrideToggle = "OverrideSoundMap"))
	TMap<EProjectileSoundType, FGeoSoundEntry> SoundMap;

	/** Team attitudes (relative to the projectile's owner) whose actors count as a valid overlap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag",
					  OverrideToggle = "OverrideOverlapAttitude"))
	int32 OverlapAttitude = TeamAttitudeMask::HostileOrNeutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (OverrideToggle = "OverrideCanOverlapInstigator"))
	bool bCanOverlapInstigator = false;

	/** Time the projectile must have been alive before it may overlap its own instigator. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (ClampMin = "0", UIMin = "0", EditCondition = "bCanOverlapInstigator", EditConditionHides,
					  OverrideToggle = "OverrideCanOverlapInstigator"))
	float LifeTimeThresholdBeforeOverlapSelf = 0.2f;
};

/**
 * Bundles a projectile class with the values it should spawn with, so a projectile class and its params always travel
 * together. Holds only the ProjectileClass and one EOverrideParam toggle per value — the values themselves are
 * inherited from FProjectileParamsBase and only show while their toggle is on OverrideValue. Applied by
 * AGeoProjectile::ApplyProjectileParams.
 */
USTRUCT(BlueprintType)
struct FExternalProjectileParams : public FProjectileParamsBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideDistanceSpan = EOverrideParam::UseGameDataSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideSpeed = EOverrideParam::UseGameDataSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideRadius = EOverrideParam::UseGameDataSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideHeadColor = EOverrideParam::KeepBlueprintDefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideTrailColor = EOverrideParam::KeepBlueprintDefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideTrailLifetimeScale = EOverrideParam::KeepBlueprintDefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideSoundMap = EOverrideParam::KeepBlueprintDefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideOverlapAttitude = EOverrideParam::KeepBlueprintDefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideCanOverlapInstigator = EOverrideParam::KeepBlueprintDefaultValue;
};
