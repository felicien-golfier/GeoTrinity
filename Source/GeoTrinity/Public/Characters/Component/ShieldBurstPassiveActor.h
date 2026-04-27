// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ShieldBurstPassiveActor.generated.h"

/**
 * Replicated actor dynamically added to the Square character while the shield burst passive ability is active.
 * The ability updates GaugeRatio on the server; replication delivers it to all clients so they can drive visuals.
 * Subclass in Blueprint to implement OnGaugeRatioChanged (gauge fill, charge arrow, timeline, etc.).
 */
UCLASS(Blueprintable, BlueprintType)
class GEOTRINITY_API AShieldBurstPassiveActor : public AActor
{
	GENERATED_BODY()

public:
	AShieldBurstPassiveActor();
	void InitializeMaterialInstances();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called server-side by the ability whenever the gauge value changes. */
	void SetGaugeRatio(float NewRatio);

protected:
	/** Fires on all clients whenever GaugeRatio changes. Override in Blueprint to update visuals. */
	UFUNCTION(BlueprintImplementableEvent)
	void OnGaugeRatioChanged(float Ratio);

private:
	UFUNCTION()
	void OnRep_GaugeRatio();

	UPROPERTY(ReplicatedUsing = OnRep_GaugeRatio)
	float GaugeRatio = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float ChargeTime;

	// Has to stay material 0;
	UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UMaterialInstanceDynamic> ShieldMaterialInstance;

	// Has to stay material 0;
	UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UMaterialInstanceDynamic> CharacterMaterialInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "ShieldBurst")
	FVector2D ActorRelativeLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "ShieldBurst")
	FName CharacterMaterialScalarParamName = "GlowGauge";
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "ShieldBurst")
	FName ShieldMaterialScalarParamName = "FillAmount";
};
