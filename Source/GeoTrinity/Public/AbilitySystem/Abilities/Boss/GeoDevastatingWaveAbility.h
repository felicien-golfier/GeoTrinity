// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoDevastatingWaveAbility.generated.h"

/**
 * Boss ability that teleports the boss to the arena center, then launches the DevastatingWavePattern
 * from that position. Teleport happens server-side inside GetFireOrigin2D so the payload Origin
 * is already the teleport destination when the pattern multicast fires.
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
	FVector2D TeleportLocation = FVector2D::ZeroVector;
};
