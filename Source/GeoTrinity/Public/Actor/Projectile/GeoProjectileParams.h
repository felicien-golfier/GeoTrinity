// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "GeoProjectileParams.generated.h"

class AGeoProjectile;

/**
 * How a single projectile spawn param resolves its value:
 * - UseGameDataSettings: read the project-wide value from UGameDataSettings. Colors and trail lifetime have no such
 *   setting, so for them this behaves exactly like KeepBlueprintDefaultValue.
 * - KeepBlueprintDefaultValue: leave the projectile's own Blueprint / Niagara-asset default untouched.
 * - OverrideValue: use the explicit value stored next to this toggle.
 */
UENUM(BlueprintType)
enum class EOverrideParam : uint8
{
	UseGameDataSettings UMETA(DisplayName = "Use Game Data Settings"),
	KeepBlueprintDefaultValue UMETA(DisplayName = "Keep Blueprint Default"),
	OverrideValue UMETA(DisplayName = "Override")
};

/**
 * A projectile's per-instance values: radius (drives both the sphere collider and the "User.Bullet_Radius" param — so it
 * is gameplay, not just visual), plus head/trail color and trail lifetime. Used directly as a projectile's per-Blueprint
 * defaults (edited in Class Defaults) and as the resolved bundle AGeoProjectile::ApplyParams pushes onto the projectile.
 * FGeoProjectileParams derives from this so a spawn's override values share one declaration with the defaults.
 */
USTRUCT(BlueprintType)
struct FProjectileParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0"))
	float Radius = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor HeadColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor TrailColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0"))
	float TrailLifetimeScale = 1.f;
};

/**
 * Bundles a projectile class with the values it should spawn with, so a projectile class and its params always travel
 * together. Inherits the per-instance values from FProjectileParams and adds a per-value EOverrideParam toggle
 * (settings / Blueprint default / explicit override) plus distance/speed. Applied by AGeoProjectile::ApplyProjectileParams.
 * Note: the inherited value fields can't carry EditConditionHides (meta can't be added to an inherited property), so they
 * show unconditionally; the toggle still decides whether the value is used.
 */
USTRUCT(BlueprintType)
struct FGeoProjectileParams : public FProjectileParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideDistanceSpan = EOverrideParam::UseGameDataSettings;
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (ClampMin = "0", UIMin = "0",
					  EditCondition = "OverrideDistanceSpan == EOverrideParam::OverrideValue", EditConditionHides))
	float DistanceSpan = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideSpeed = EOverrideParam::UseGameDataSettings;
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (ClampMin = "0", UIMin = "0", EditCondition = "OverrideSpeed == EOverrideParam::OverrideValue",
					  EditConditionHides))
	float ProjectileSpeed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideRadius = EOverrideParam::UseGameDataSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideHeadColor = EOverrideParam::KeepBlueprintDefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideTrailColor = EOverrideParam::KeepBlueprintDefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOverrideParam OverrideTrailLifetimeScale = EOverrideParam::KeepBlueprintDefaultValue;
};
