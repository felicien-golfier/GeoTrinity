// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoDevastatingWaveAbility.generated.h"

/**
 * Boss ability that teleports the boss to an AGeoTargetPoint, then launches the DevastatingWavePattern
 * from that position. The destination is the first point in the boss's own arena carrying TeleportLocationTag,
 * resolved server-side inside GetFireOrigin2D so the payload Origin is already the teleport destination
 * when the pattern multicast fires.
 */
UCLASS()
class GEOTRINITY_API UGeoDevastatingWaveAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	/** Returns the 2D location of the arena's TeleportLocationTag point; the wave pattern teleports the boss there before it starts. */
	virtual FVector2D GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
									  int Seed) const override;

private:
	/** Which of the arena's points to land on (TargetPoint.*); the arena itself comes from the boss. */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	FGameplayTag TeleportLocationTag;
};
