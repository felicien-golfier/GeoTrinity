// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoHealingZone.generated.h"


/**
 * Deployable healing zone placed by the Circle player.
 * Periodically heals allies who remain inside the capsule area. Can be absorbed by UGeoMoiraBeamAbility
 * which drains its health and converts it into fuel, radius growth, and damage/heal boost.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoHealingZone : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoHealingZone();
	virtual void InitInteractable(FInteractableActorData* Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float GetDurationPercent() const override;

protected:
	virtual FDeployableData const* GetData() const override { return &Data; }

	virtual void InitDrain() override;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnRep_Data() const;

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	FDeployableData Data;

	TSet<TWeakObjectPtr<AActor>> ActorsInZone;
};
