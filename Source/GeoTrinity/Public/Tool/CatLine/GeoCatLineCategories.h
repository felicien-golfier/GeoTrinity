// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GeoCatLineCategories.generated.h"

/**
 * The picked categories of Data/CatLines.txt, stored by name (a position would shift when the file's categories
 * change). Empty = every category. Edited by FGeoCatLineCategoriesCustomization as a dropdown of checkboxes.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoCatLineCategories
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoCatLine")
	TArray<FName> Names;
};
