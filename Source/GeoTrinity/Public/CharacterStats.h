#pragma once
#include "CoreMinimal.h"

#include "CharacterStats.generated.h"

USTRUCT( BlueprintType )
struct GEOTRINITY_API FCharacterStats : public FTableRowBase
{

	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	float Speed = 300.f;
};
