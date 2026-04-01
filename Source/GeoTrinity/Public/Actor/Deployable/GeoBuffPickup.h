// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Tool/UGameplayLibrary.h"

#include "GeoBuffPickup.generated.h"

class UCurveFloat;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

USTRUCT()
struct FBuffPickupData : public FDeployableData
{
	GENERATED_BODY()

	/** Index into BuffMeshAssets (and the ability's BuffEffectDataAssets). -1 = none selected. */
	UPROPERTY(Transient)
	int32 MeshIndex = -1;

	/** World position the pickup travels to after spawning. */
	UPROPERTY(Transient)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float PowerScale = 1.f;
};

/**
 * Pickup that grants a buff to the player who collects it.
 * Spawned by Triangle's reload ability. Initialized via InitInteractableData before BeginPlay.
 *
 * Fill BuffMeshAssets in the Blueprint Class Defaults to match the ability's BuffEffectDataAssets:
 * index N in BuffMeshAssets = index N in BuffEffectDataAssets (same buff type).
 */
UCLASS()
class GEOTRINITY_API AGeoBuffPickup : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoBuffPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitInteractableData(FInteractableActorData* InputData) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual FBuffPickupData const* GetData() const override { return &Data; }

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	FBuffPickupData Data;

	/**
	 * One static mesh asset per buff type. Must match the ability's BuffEffectDataAssets by index.
	 * The active mesh is swapped on BuffMeshComponent at runtime based on the selected index.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup|Appearance")
	TArray<TObjectPtr<UStaticMesh>> BuffMeshAssets;

private:
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
				   int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UFUNCTION()
	void OnRep_Data();

	void UpdateMesh();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BuffMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup|Appearance")
	float RotationSpeed = 90.f;

	/** Curve controlling the launch movement. X = time (seconds), Y = lerp alpha (0 to 1). */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup|Appearance")
	TObjectPtr<UCurveFloat> LaunchCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup|Appearance")
	float MinScale = .5f;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup|Appearance")
	float MaxScale = 1.5f;

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 OverlapAttitude = static_cast<int32>(ETeamAttitudeBitflag::Friendly);

	bool bMovingToTarget = true;
	float LaunchElapsedTime = 0.f;
	FVector LaunchStartLocation = FVector::ZeroVector;
};
