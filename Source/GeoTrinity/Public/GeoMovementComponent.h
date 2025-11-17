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

private:
	AGeoCharacter* GetGeoCharacter() const;
};
