// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/GeoFXMoment.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GeoBuffFXDataAsset.generated.h"

/**
 * One buff attribute and the FX it shows while it sits above its base value on a character. Read by every
 * UGeoFXComponent, each taking its own side: a character's UGeoGameFeelComponent wears CharacterFX, and the shots it
 * fires wear ProjectileFX through their UGeoProjectileFXComponent. Either may be left empty to show the buff on one
 * side only.
 *
 * Sustained on both sides: a buff is turned on when the attribute rises and off again when it falls back, so its system
 * stays attached and its sound loops for exactly as long as the buff lasts. Its magnitude curve is re-sampled on every
 * change of the attribute, so a buff can grow with how boosted it is.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoBuffFXEntry
{
	GENERATED_BODY()

	/** Watched on the buffed character's ASC — above its base value is what "buffed" means here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute Attribute;

	/** Worn by the buffed character itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoSustainedFXMoment CharacterFX;

	/** Worn by a projectile the buffed character fires. Only the damage and applied-heal boosts reach a shot at all,
	 * and only on a shot that carries the matching effect — a damage buff must not light up a heal shot. Set on any
	 * other attribute it stays unused; that buff shows on the character alone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoSustainedFXMoment ProjectileFX;
};

/**
 * Every buff the game shows, project-wide: one entry per attribute. Pointed at by UGameDataSettings::BuffFX and driven
 * by UGeoFXComponent.
 *
 * An asset rather than a Config array on the settings object, because a moment holds hard asset references: a
 * UPROPERTY(Config) is read by LoadConfig while the settings CDO is constructed, which would load every buff system and
 * sound during engine startup. Going through one TSoftObjectPtr — the same shape as AbilityInfo and PlayerClassData —
 * defers that to the first character that needs it, and lets a moment hold its systems hard so nothing re-resolves a
 * soft path on every attribute change.
 */
UCLASS(BlueprintType)
class GEOTRINITY_API UGeoBuffFXDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoBuffFX")
	TArray<FGeoBuffFXEntry> Entries;
};
