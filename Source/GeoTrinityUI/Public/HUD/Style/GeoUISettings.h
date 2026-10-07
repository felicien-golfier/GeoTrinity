// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "GeoUISettings.generated.h"

class UGeoUITheme;

/**
 * Project Settings panel (Geo UI) naming the theme every widget reads. Lives in the UI module, so the gameplay
 * settings never name a UI type.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Geo UI"))
class GEOTRINITYUI_API UGeoUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Fonts, colours and default frame of every themed widget. */
	UPROPERTY(Config, EditAnywhere, Category = "GeoUI")
	TSoftObjectPtr<UGeoUITheme> Theme;
};
