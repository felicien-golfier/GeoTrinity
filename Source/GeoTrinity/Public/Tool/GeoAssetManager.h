// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "GeoAssetManager.generated.h"

/**
 * Deriving class to enable the addition of gameplay tags (among others)
 */
UCLASS()
class GEOTRINITY_API UGeoAssetManager : public UAssetManager
{
	GENERATED_BODY()
public:
	static UGeoAssetManager& Get();
protected:
	virtual void StartInitialLoading() override;

};
