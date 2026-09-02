// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoCueParam.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameplayEffectTypes.h"

FGameplayCueParameters FGeoCueParam::MakeCueParams(AActor* Instigator, AActor* EffectCauser, FVector const Location,
												   int32 const AbilityLevel, FGameplayTag const AbilityTag) const
{
	// Global ::IsValid, not this struct's own IsValid().
	AActor* const InstigatorAvatar = GeoASLib::GetAvatarFromActor(Instigator);
	ensureMsgf(!::IsValid(Instigator) || ::IsValid(InstigatorAvatar),
			   TEXT("%hs: cue %s instigator %s has no ASC to resolve an avatar from"), __FUNCTION__,
			   *CueTag.ToString(), *GetNameSafe(Instigator));

	FGameplayCueParameters CueParams;
	CueParams.Instigator = InstigatorAvatar;
	CueParams.EffectCauser = EffectCauser;
	CueParams.Location = Location;
	CueParams.AbilityLevel = AbilityLevel;
	CueParams.GameplayEffectLevel = static_cast<int32>(Color);
	if (SoundTag.IsValid())
	{
		CueParams.AggregatedSourceTags.AddTag(SoundTag);
	}
	if (AbilityTag.IsValid())
	{
		CueParams.SourceObject = GeoASLib::GetAbilityCDO(AbilityTag);
	}
	return CueParams;
}

FGameplayCueParameters FGeoCueParam::MakeCueParams(FAbilityPayload const& Payload, FVector const Location) const
{
	return MakeCueParams(Payload.Instigator, Payload.Instigator, Location, Payload.AbilityLevel, Payload.AbilityTag);
}
