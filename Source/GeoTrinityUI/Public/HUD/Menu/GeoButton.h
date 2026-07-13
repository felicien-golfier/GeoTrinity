// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Button.h"
#include "CoreMinimal.h"

#include "GeoButton.generated.h"

/**
 * Button whose Slate widget treats keyboard/gamepad focus as hover, so focus navigation goes through
 * the exact same path as mouse-over: same style brushes, hover sound and OnHovered/OnUnhovered delegates.
 */
UCLASS()
class GEOTRINITYUI_API UGeoButton : public UButton
{
	GENERATED_BODY()

protected:
	/** Builds an SGeoButton (focus → hover mapping) in place of the default SButton. */
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
