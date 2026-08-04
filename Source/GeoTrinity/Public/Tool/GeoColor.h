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
	Damage,
	AllyDamage,
	LethalDamage,
	Heal,
	Shield,
	BothHealAndDamage,
	DamageReduction,
	DamageBoost,
	HealBoost,
	MoveSpeed,
	Neutral,
	DeployableBlockingAll,
	DeployableBlockingEnemies,
	DeployableBlockingAllies,
	DeployableNotBlocking,
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

	/** Default-constructs to Override mode with FLinearColor::White. */
	FGeoColorParam() = default;
	/** Constructs an explicit-override param (Color = Override) using InOverrideColor as the literal color. */
	explicit FGeoColorParam(FLinearColor const& InOverrideColor) : OverrideColor(InOverrideColor) {}

	/**
	 * The palette color configured for Color, or OverrideColor when Color is Override.
	 *
	 * @param Alpha  Alpha to apply to the returned color. Negative keeps the palette color's own alpha unchanged.
	 */
	FLinearColor GetColor(float Alpha = -1.f) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EGeoColor Color = EGeoColor::Override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (EditCondition = "Color == EGeoColor::Override", EditConditionHides))
	FLinearColor OverrideColor = FLinearColor::White;
};

class UTexture2D;

namespace GeoColor
{
	/** Number of palette slots: Override is last and carries no color of its own, so its ordinal is the count. */
	constexpr int32 SlotCount = static_cast<int32>(EGeoColor::Override);

	/** Palette texture parameter of the outline post-process, and the texel count it needs to turn a stencil index into
	 * a UV. Written by AGeoGameCamera, authored by AI/Python/make_deployable_outline_material.py — the two must agree.
	 */
	inline FName const PaletteTextureParam = TEXT("Palette");
	inline FName const PaletteSizeParam = TEXT("PaletteSize");

	/**
	 * Builds ColorPalette into a one-pixel-tall texture, one texel per slot at its own EGeoColor ordinal — the form the
	 * outline post-process reads, since a material can index a texture but not a name. Transient and rebuilt per level,
	 * so retinting stays a Game Data Settings edit with no asset to resave, and adding a slot needs no material work.
	 */
	GEOTRINITY_API UTexture2D* CreatePaletteTexture();
} // namespace GeoColor
