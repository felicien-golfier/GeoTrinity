// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GeoActorFilter.generated.h"

class AGeoCharacter;

/** Which characters a trigger watches: every character of Class (APlayableCharacter = the players) carrying Tag. */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoActorFilter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TSubclassOf<AGeoCharacter> Class;

	/** Actor tag the character must carry (an enemy setup's Tag, telling apart variants of one class). None = any. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FName Tag;

	/** Returns true when Actor is of Class and carries Tag (None = any). */
	bool Matches(AActor const& Actor) const;
};
