// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayableCharacter.h"
#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoOverlayWidget.generated.h"

class UGeoAbilityBarWidget;
class AGeoHUD;

/**
 * Root player overlay widget. Holds the bottom-center ability bar as a BindWidget so the HUD can rebuild it directly
 * from C++ (AGeoHUD::BuildAbilityBar) without any Blueprint event-graph wiring. WBP_MainOverlay reparents to this
 * class and places a WBP_AbilityBar named "AbilityBar" anchored bottom-center.
 */
UCLASS()
class GEOTRINITYUI_API UGeoOverlayWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuilds the ability bar from the HUD's current ability set. Called by AGeoHUD::BuildAbilityBar. */
	void BuildAbilityBar(AGeoHUD* GeoHUD, APlayableCharacter* PlayableCharacter);

protected:
	/** Bottom-center ability bar. Bound from WBP_MainOverlay; rebuilt by the HUD when abilities are granted/changed. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoAbilityBarWidget> AbilityBar;
};
