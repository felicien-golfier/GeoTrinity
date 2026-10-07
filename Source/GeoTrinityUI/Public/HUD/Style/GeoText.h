// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/TextBlock.h"
#include "CoreMinimal.h"
#include "HUD/Style/GeoUITheme.h"

#include "GeoText.generated.h"

/** A TextBlock whose font, colour and case come from its role in the UI theme, so all text of a role changes together. */
UCLASS()
class GEOTRINITYUI_API UGeoText : public UTextBlock
{
	GENERATED_BODY()

public:
	/** Applies Role's style, then the TextBlock's own properties. */
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

	/** Picks the theme style; Custom keeps the font and colour set on this widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoText")
	EGeoTextRole Role = EGeoTextRole::Body;
};
