// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tool/GeoColor.h"

#include "GeoCueParam.generated.h"

struct FAbilityPayload;
struct FGameplayCueParameters;

/**
 * One authored gameplay cue: its tag and the palette slot its VFX draws in.
 * Every cue fired from C++ config (deployable moments, pattern init/start, ability fire, effect application) is typed
 * as this, so a designer sees the knobs of a cue in one place instead of parallel tag and color fields.
 *
 * A cue is what carries data into a notify Blueprint (a recall beam's two points, a pattern's timing). A moment that
 * only spawns a system and plays sounds is an FGeoBurstFXMoment instead, authored where the moment happens.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoCueParam
{
	GENERATED_BODY()

	/** True when this entry plays anything at all. */
	bool IsValid() const { return CueTag.IsValid(); }

	/**
	 * Builds the parameters this cue is fired with: who caused it, where it happens, and this entry's own palette slot.
	 * The single builder every Geo cue goes through — layer the cue's own data (Normal, RawMagnitude,
	 * NormalizedMagnitude) onto the result at the call site.
	 *
	 * Instigator is always resolved to its avatar actor, so a cue notify never has to tell a PlayerState from the pawn
	 * whose ASC it owns.
	 *
	 * @param Instigator    Actor responsible for the cue; a PlayerState resolves to its pawn.
	 * @param EffectCauser  What visibly produced it: the deployable, the projectile, or the caster itself.
	 * @param AbilityTag    Ability behind the cue — its CDO travels as SourceObject. Invalid for a cue with no ability.
	 */
	FGameplayCueParameters MakeCueParams(AActor* Instigator, AActor* EffectCauser, FVector Location,
										 int32 AbilityLevel, FGameplayTag AbilityTag) const;

	/** MakeCueParams for a cue fired by an ability: the payload's avatar both instigates and causes it. */
	FGameplayCueParameters MakeCueParams(FAbilityPayload const& Payload, FVector Location) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	// Palette slot the cue's VFX draws in. Travels to the cue Blueprint as GameplayEffectLevel, which
	// GeoLib::GetPaletteColorFromIndex turns back into a color — not packed into Normal, which callers already use for
	// their own data (recall direction, pattern timing). Override is not a valid choice: it has no palette slot.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EGeoColor Color = EGeoColor::Neutral;

	// MIGRATION SHIM — delete with FGeoSoundRow once the sounds are re-authored as moments.
	// Read-only: nothing plays it any more. It exists so the tagged property still deserializes out of the assets that
	// authored it, instead of being dropped the next time one of them is saved.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Event.Sound"))
	FGameplayTag SoundTag;
};
