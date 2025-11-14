#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"

#include "GeoMovementComponent.generated.h"

class AGeoPawn;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
public:
	UGeoMovementComponent();
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	void ApplyControlInputToVelocity(float DeltaTime);
	bool LimitWorldBounds();
	virtual float GetMaxSpeed() const override { return MaxSpeed; }

	/** Maximum velocity magnitude allowed for the controlled Pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloatingPawnMovement)
	float MaxSpeed = 1000.f;

	/** Acceleration applied by input (rate of change of velocity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloatingPawnMovement)
	float Acceleration = 1000.f;

	/** Deceleration applied when there is no input (rate of change of velocity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloatingPawnMovement)
	float Deceleration = 400.f;

	/**
	 * Setting affecting extra force applied when changing direction, making turns have less drift and become more
	 * responsive. Velocity magnitude is not allowed to increase, that only happens due to normal acceleration. It may
	 * decrease with large direction changes. Larger values apply extra force to reach the target direction more
	 * quickly, while a zero value disables any extra turn force.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FloatingPawnMovement, meta = (ClampMin = "0", UIMin = "0"))
	float TurningBoost = 8.0f;

private:
	AGeoPawn* GetGeoPawn() const;
};
