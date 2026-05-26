// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

class AGeoPillar;

#include "DevastatingWavePattern.generated.h"

/**
 * Expanding radial wave fired from the boss's center position.
 * Hits each interactable actor exactly once as the wave radius passes through them.
 * Pillars are recalled (triggering their explosion); all other hostiles receive WaveEffectDataArray.
 * Runs identically on all clients via PatternStartMulticast; server-side hit detection only.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UDevastatingWavePattern : public UTickablePattern
{
	GENERATED_BODY()

protected:
	virtual void StartPattern() override;
	virtual void TickPattern(float ServerTime, float SpentTime) override;

private:
	bool ShouldHitActor(AActor const* Actor) const;
#if WITH_EDITOR
	void DrawDebugSafeZones() const;
#endif
	;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	float ExpansionSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	float MaxRadius = 3000.f;

	TSet<TWeakObjectPtr<AActor>> HitActors;
	TArray<TPair<FVector2D, float>> PillarsLocationAndRadius;
};
