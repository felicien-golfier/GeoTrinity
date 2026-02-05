#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoBuffPickup.generated.h"

class USphereComponent;
class UPrimitiveComponent;

/**
 * Pickup that grants a buff to the player who collects it.
 * Spawned by Triangle's reload ability. Effect data is provided by the spawning ability.
 */
UCLASS()
class GEOTRINITY_API AGeoBuffPickup : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoBuffPickup();

	virtual void Init() override;
	virtual void End() override;

	/** Set effect data and visual power scale (based on missing ammo). Call after Init. */
	void Setup(TArray<TInstancedStruct<FEffectData>> const& InEffectData, float InPowerScale);

private:
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						 int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionSphere;

	TArray<TInstancedStruct<FEffectData>> EffectDataArray;
	float PowerScale = 1.f;
};
