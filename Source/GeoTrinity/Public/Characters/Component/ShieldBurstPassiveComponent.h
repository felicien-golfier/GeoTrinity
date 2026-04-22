// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "ShieldBurstPassiveComponent.generated.h"

/**
 * Replicated component dynamically added to the Square character while the shield burst passive ability is active.
 * The ability updates GaugeRatio on the server; replication delivers it to all clients so they can drive visuals.
 * Subclass in Blueprint to implement OnGaugeRatioChanged (gauge fill, charge arrow, timeline, etc.).
 */
UCLASS(Blueprintable, BlueprintType)
class GEOTRINITY_API UShieldBurstPassiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShieldBurstPassiveComponent();
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
};
