// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoGameFeelComponent.generated.h"

class UMeshComponent;
class UMaterialInterface;

/**
 * Centralizes cosmetic game feel reactions (hit flash, recoil) for any actor.
 * Add to AGeoCharacter and AGeoInteractableActor subclasses.
 * Auto-discovers the owner's first mesh on BeginPlay.
 */
UCLASS(ClassGroup = "GeoTrinity", meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoGameFeelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGeoGameFeelComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** Flashes the owner's mesh with HitFlashMaterial for HitFlashDuration seconds. Uses LocalPlayerHitFlashMaterial when owner is the local player. */
	void FlashOnHit();

	/**
	 * Kicks the mesh backward opposite to Yaw by Distance cm, then springs back automatically.
	 * @param Distance  How far the mesh snaps back in cm.
	 */
	void ApplyRecoil(float Distance);

	UPROPERTY(EditDefaultsOnly, Category = "Recoil", meta = (ClampMin = "0"))
	float RecoilRecoverySpeed = 14.f;

private:
	UPROPERTY()
	TObjectPtr<UMeshComponent> TargetMesh;

	FTimerHandle HitFlashTimerHandle;

	FVector CurrentRecoilOffset = FVector::ZeroVector;
	FVector InitialMeshRelativeLocation = FVector::ZeroVector;
};
