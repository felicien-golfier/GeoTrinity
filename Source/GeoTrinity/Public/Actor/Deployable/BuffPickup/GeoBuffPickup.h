// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoBuffPickup.generated.h"

class UCurveFloat;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

USTRUCT()
struct FBuffPickupData : public FDeployableData
{
	GENERATED_BODY()

	/** Index of the chosen buff. Used to look up the pickup tint in the reload ability CDO's BuffColors palette
	 * (resolved via Data.AbilityTag). -1 = none selected (no color applied). */
	UPROPERTY(Transient)
	int32 BuffIndex = -1;

	/** World position the pickup travels to after spawning. */
	UPROPERTY(Transient)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float PowerScale = 1.f;
};

/**
 * Pickup that grants a buff to the player who collects it.
 * Spawned by Triangle's reload ability. Initialized via InitInteractable before BeginPlay.
 *
 * A single mesh is set on BuffMeshComponent in the Blueprint; the buff type is conveyed by tinting the mesh's dynamic
 * material with the color the reload ability CDO maps the chosen buff index to (UGeoReloadAbility::BuffColors).
 */
UCLASS()
class GEOTRINITY_API AGeoBuffPickup : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoBuffPickup();

	/** Registers Data with COND_InitialOnly — pickup state is set once and never updated. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** Casts InputData to FBuffPickupData and initializes spawn target, mesh index, and PowerScale before BeginPlay. */
	virtual void InitInteractable(FInteractableActorData* InputData) override;
	/** Binds the overlap delegate, sets visual scale from PowerScale, and starts launch movement toward TargetLocation. */
	virtual void BeginPlay() override;
	/** Rotates the mesh and advances the LaunchCurve lerp toward TargetLocation each tick. */
	virtual void Tick(float DeltaTime) override;

protected:
	virtual FBuffPickupData const* GetData() const override { return &Data; }

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	FBuffPickupData Data;

	/** Vector parameter on BuffMeshComponent's material that the buff color is written to. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Appearance")
	FName ColorParameterName = TEXT("Color");

private:
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
				   int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UFUNCTION()
	void OnRep_Data();

	/** Creates the dynamic material instance if needed and tints it with the buff color resolved from the reload
	 * ability CDO for Data.BuffIndex. */
	void UpdateColor();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BuffMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BuffMaterialInstance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Appearance", meta = (AllowPrivateAccess = true))
	float RotationSpeed = 90.f;

	/** Curve controlling the launch movement. X = time (seconds), Y = lerp alpha (0 to 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Appearance", meta = (AllowPrivateAccess = true))
	TObjectPtr<UCurveFloat> LaunchCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Appearance", meta = (AllowPrivateAccess = true))
	float MinScale = .5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Appearance", meta = (AllowPrivateAccess = true))
	float MaxScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag", AllowPrivateAccess = true))
	int32 OverlapAttitude = TeamAttitudeMask::Friendly;

	bool bMovingToTarget = true;
	float LaunchElapsedTime = 0.f;
	FVector LaunchStartLocation = FVector::ZeroVector;
};
