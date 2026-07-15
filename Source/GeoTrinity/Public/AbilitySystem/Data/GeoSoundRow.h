// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GeoSoundRow.generated.h"


class USoundBase;

/** DataTable row mapping a GameplayCue tag to the sound to play. */
USTRUCT(BlueprintType)
struct FGeoSoundRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Volume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D PitchRandomMinMax = FVector2D::One();
};

UCLASS()
class GEOTRINITY_API UGeoSoundRowLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the first row whose Tag matches Tag; bFound is false and a default row is returned if none. */
	UFUNCTION(BlueprintPure, Category = "GeoTrinity|Sound", meta = (DataTablePin = "SoundTable"))
	static FGeoSoundRow FindSoundForTag(UDataTable const* SoundTable, FGameplayTag Tag, bool& bFound);
};
