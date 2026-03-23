#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GeoMovementComponent.generated.h"

class AGeoCharacter;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UGeoMovementComponent();

	virtual void BeginPlay() override;
	void ApplySpeedMultiplier(float Multiplier);

private:
	float BaseMaxWalkSpeed = 0.f;
	float BaseMaxAcceleration = 0.f;

	AGeoCharacter* GetGeoCharacter() const;
};
