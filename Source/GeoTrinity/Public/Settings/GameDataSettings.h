// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "AbilitySystem/Data/StatusInfo.h"
#include "Engine/DeveloperSettings.h"
#include "GameDataSettings.generated.h"

class UStatusInfo;
/**
 * A place to link all data tables for game stuff, accessible by server and client
 */
UCLASS(Config=Game, defaultconfig, meta = (DisplayName="Game Data Settings"))
class GEOTRINITY_API UGameDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	
	template <typename T>
	static T* GetLoadedDataAsset(const TSoftObjectPtr<T>& SoftObject);
	
	/* Soft path will be converted to content reference before use */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", AdvancedDisplay)
	TSoftObjectPtr<UStatusInfo> StatusInfo;

};

template <typename T>
T* UGameDataSettings::GetLoadedDataAsset(const TSoftObjectPtr<T>& SoftObject)
{
	return SoftObject.LoadSynchronous();
}
