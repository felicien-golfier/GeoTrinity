#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GeoShapes.h"
#include "InputStep.h"

#include "GeoMovementComponent.generated.h"

class AGeoPawn;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
public:
	UGeoMovementComponent();

	// Applies simple 2D movement along the pawn forward/right vectors
	void MovePawnWithInput(float DeltaTime, FVector2D GivenMovementInput);

	// Simple AABB collision resolution against an obstacle box
	void ApplyCollision(const FBox2D& Obstacle) const;

private:
	AGeoPawn* GetGeoPawn() const;
};
