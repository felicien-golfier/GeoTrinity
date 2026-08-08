// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tool/GeoColor.h"

#include "GeoCueParam.generated.h"

struct FGameplayCueParameters;

/**
 * One authored gameplay cue: its tag, the palette slot its VFX draws in, and the sound it plays.
 * Every cue fired from C++ config (deployable moments, pattern init/start) is typed as this, so a designer sees the
 * three knobs of a cue in one place instead of parallel tag and color fields.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoCueParam
{
	GENERATED_BODY()

	/** Writes Color's palette slot and SoundTag into CueParams. Call on the caller-built params before executing. */
	void FillCueParams(FGameplayCueParameters& CueParams) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	// Palette slot the cue's VFX draws in. Travels to the cue Blueprint as GameplayEffectLevel, which
	// GeoLib::GetPaletteColorFromIndex turns back into a color — not packed into Normal, which callers already use for
	// their own data (recall direction, pattern timing). Override is not a valid choice: it has no palette slot.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EGeoColor Color = EGeoColor::Neutral;

	/** DT_GenericSound row to play, passed through AggregatedSourceTags for UGeoGenericSoundCueNotify to look up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Event.Sound"))
	FGameplayTag SoundTag;
};
