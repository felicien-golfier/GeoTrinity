// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "PatternAbilityParams.generated.h"

/**
 * What the firer of a pattern ability decides for one activation; the ability does the rest with it. Given as the
 * activation's TriggerEventData TargetData (an encounter firing), else UPatternAbility makes it from the caster's
 * arena. Server only, never replicated.
 */
USTRUCT()
struct GEOTRINITY_API FPatternAbilityParams : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	/** The room (Arena.*) whose AGeoTargetPoint the ability's TargetPointTag names. */
	UPROPERTY()
	FGameplayTag ArenaTag;

	/** Pattern data this activation launches with; unset = the ability's own. */
	UPROPERTY()
	TInstancedStruct<FPatternData> PatternData;

	/** Required by FGameplayAbilityTargetData; returns this struct's UScriptStruct for RPC serialization. */
	virtual UScriptStruct* GetScriptStruct() const override { return FPatternAbilityParams::StaticStruct(); }
};
