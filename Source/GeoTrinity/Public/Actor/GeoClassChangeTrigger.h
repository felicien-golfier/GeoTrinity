// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoClassChangeTrigger.generated.h"

class USphereComponent;

UCLASS()
class GEOTRINITY_API AGeoClassChangeTrigger : public AActor
{
	GENERATED_BODY()

public:
	AGeoClassChangeTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	EPlayerClass TargetClass = EPlayerClass::None;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
};
