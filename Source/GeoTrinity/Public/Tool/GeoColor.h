// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GeoColor.generated.h"

/**
 * Semantic color slots of the game. Every slot but Override resolves through UGameDataSettings::ColorPalette, so
 * retinting a gameplay meaning across the whole game is a single edit there. Override is the escape hatch: the owning
 * FGeoColorParam then uses its own OverrideColor.
 */
UENUM(BlueprintType)
enum class EGeoColor : uint8
{
	DirectDamage,
	DamageOverTime,
	LethalDamage,
	DirectHeal,
	HealOverTime,
	Shield,
	BothHealAndDamageOverTime,
	DamageReduction,
	DamageBoost,
	HealBoost,
	MoveSpeed,
	/** Danger telegraphed before it lands: wind-up tints, pre-fire danger zones. */
	Telegraph,
	Override
};

/**
 * Every authored color param in the game. Picks a palette slot, or an explicit color when Color is Override — the only
 * case where OverrideColor is shown at all. Always read through GetColor(), never OverrideColor directly.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoColorParam
{
	GENERATED_BODY()

	FGeoColorParam() = default;
	explicit FGeoColorParam(FLinearColor const& InOverrideColor) : OverrideColor(InOverrideColor) {}

	/** The palette color configured for Color, or OverrideColor when Color is Override. */
	FLinearColor GetColor() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EGeoColor Color = EGeoColor::Override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (EditCondition = "Color == EGeoColor::Override", EditConditionHides))
	FLinearColor OverrideColor = FLinearColor::White;
};
