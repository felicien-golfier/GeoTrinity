// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"

#include "GeoGenericSoundCueNotify.generated.h"


class UDataTable;

/**
 * Generic sound GameplayCue handler: for each tag in the cue's AggregatedSourceTags, looks up the matching
 * FGeoSoundRow in SoundTable and plays its entry through UGeoSoundRowLibrary (2D, audience-gated,
 * instigator-relative volume and pitch).
 * Fire it with the GenericGameplayCueSoundTag from UGameDataSettings and the sound tag(s) in AggregatedSourceTags,
 * with CueParameters.Instigator set to the actor the sound belongs to.
 */
UCLASS()
class GEOTRINITY_API UGeoGenericSoundCueNotify : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:
	virtual bool OnExecute_Implementation(AActor* Target, FGameplayCueParameters const& Parameters) const override;

	/** Sound-tag → FGeoSoundEntry lookup table (FGeoSoundRow rows). */
	UPROPERTY(EditDefaultsOnly, meta = (RequiredAssetDataTags = "RowStructure=/Script/GeoTrinity.GeoSoundRow"))
	TObjectPtr<UDataTable> SoundTable;
};
