// Fill out your copyright notice in the Description page of Project Settings.


#include "Tool/GeoAssetManager.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"

UGeoAssetManager& UGeoAssetManager::Get()
{
	check(GEngine);
	return *Cast<UGeoAssetManager>(GEngine->AssetManager);
}

void UGeoAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	FGeoGameplayTags::InitializeNativeGameplayTags();

	// If we ever want to make Target Data (like to poll mouse-targeted actor in a GA), we will need that.
	// It's possible we simply won't, as a direction might be enough.
	// UAbilitySystemGlobals::Get().InitGlobalData();
}
