// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayTagContainer.h"

#include "GeoWorldSettings.generated.h"

/** Default KillZ, just under the shared floor (Z = 0, characters stand at ArbitraryCharacterZ): nothing in this game
 * is ever under 0. */
constexpr float GeoKillZ = -50.f;

UCLASS()
class GEOTRINITY_API AGeoWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	/** Sets KillZ to GeoKillZ, so a character off the floor dies before it falls out of sight. */
	AGeoWorldSettings(FObjectInitializer const& ObjectInitializer);

	/** When set to anything other than None, all players start with this class regardless of slot order. */
	UPROPERTY(EditAnywhere, Category = "GeoTrinity")
	EPlayerClass StartingClassOverride = EPlayerClass::None;

	/** Class abilities this map does not grant at startup; an encounter grants them later (GeoUnlockAbilityAction). */
	UPROPERTY(EditAnywhere, Category = "GeoTrinity", meta = (Categories = "Ability.Spell"))
	FGameplayTagContainer LockedAbilityTags;
};
