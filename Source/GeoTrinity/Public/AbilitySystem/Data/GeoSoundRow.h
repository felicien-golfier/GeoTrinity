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
};

UCLASS()
class GEOTRINITY_API UGeoSoundRowLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the Sound of the first row whose Tag matches Tag, or nullptr if none. */
	UFUNCTION(BlueprintPure, Category = "GeoTrinity|Sound", meta = (DataTablePin = "SoundTable"))
	static USoundBase* FindSoundForTag(UDataTable const* SoundTable, FGameplayTag Tag);
};
