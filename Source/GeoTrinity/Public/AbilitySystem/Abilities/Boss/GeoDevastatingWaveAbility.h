// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoDevastatingWaveAbility.generated.h"

/**
 * Boss ability that teleports the boss to an AGeoTargetPoint, then launches the DevastatingWavePattern
 * from that position. The destination is the first AGeoTargetPoint tagged with TeleportLocationTag,
 * resolved server-side inside GetFireOrigin2D so the payload Origin is already the teleport destination
 * when the pattern multicast fires.
 */
UCLASS()
class GEOTRINITY_API UGeoDevastatingWaveAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	virtual FVector2D GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
									  int Seed) const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	FGameplayTag TeleportLocationTag;
};
