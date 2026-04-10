#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GeoMovementComponent.generated.h"

class AGeoCharacter;

/**
 * Custom movement component that caches the character's base walk speed and acceleration on BeginPlay
 * so that attribute-driven multipliers can be applied and restored correctly.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UGeoMovementComponent();

	virtual void BeginPlay() override;

	/**
	 * Scales MaxWalkSpeed and MaxAcceleration by Multiplier relative to their cached base values.
	 *
	 * @param Multiplier  Scaling factor. 1.0 = base speed, 2.0 = double speed.
	 */
	void ApplySpeedMultiplier(float Multiplier);

private:
	float BaseMaxWalkSpeed = 0.f;
	float BaseMaxAcceleration = 0.f;

	AGeoCharacter* GetGeoCharacter() const;
};
