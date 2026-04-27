// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ShieldBurstPassiveComponent.generated.h"

/**
 * Replicated actor dynamically added to the Square character while the shield burst passive ability is active.
 * The ability updates GaugeRatio on the server; replication delivers it to all clients so they can drive visuals.
 * Subclass in Blueprint to implement OnGaugeRatioChanged (gauge fill, charge arrow, timeline, etc.).
 */
UCLASS(Blueprintable, BlueprintType)
class GEOTRINITY_API UShieldBurstPassiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShieldBurstPassiveComponent();
	/** Finds the owner's mesh material at slot 0 and creates a dynamic material instance used to drive the gauge and charge shader parameters. */
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

	UFUNCTION()
	void Charge();

	float StartChargeTime = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_GaugeRatio)
	float GaugeRatio = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float ChargeTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float DischargeTime = .3f;

	// Has to stay material 0;
	UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UMaterialInstanceDynamic> CharacterMaterialInstance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "ShieldBurst")
	FName GaugeScalarParamName = "GlowGauge";
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "ShieldBurst")
	FName ChargeScalarParamName = "FullGlowGauge";
};
