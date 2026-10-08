// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoAbilityCardWidget.h"

#include "GeoAbilityDetailWidget.generated.h"

class UGeoFrameStyle;
class UUniformGridPanel;
enum class EActiveTimerReturnType : uint8;

/**
 * The drawer the abilities page slides in from the right when a card is picked: the card's header and description,
 * then the ability's stats (FGameplayAbilityInfo::GetResolvedStats) in a StatColumns-wide grid of framed cells whose
 * texts wrap. Slides in over SlideDuration on a Slate active timer; the page closes it on a click outside.
 * Required in the BP hierarchy, besides the card's: UUniformGridPanel "StatGrid".
 */
UCLASS()
class GEOTRINITYUI_API UGeoAbilityDetailWidget : public UGeoAbilityCardWidget
{
	GENERATED_BODY()

public:
	/** Also fills StatGrid with Info's stats. */
	virtual void SetAbility(FPlayersGameplayAbilityInfo const& Info, int32 AbilityLevel,
							FLinearColor const& ClassColor) override;

	/** Shows the drawer, sliding in from its right edge unless already open. */
	void Open();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> StatGrid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityDetail", meta = (ClampMin = "1", ClampMax = "8"))
	int32 StatColumns = 2;

	/** Frame around each stat cell. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityDetail")
	TObjectPtr<UGeoFrameStyle> StatFrameStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityDetail")
	FMargin StatPadding = FMargin(18.f, 14.f);

	/** Seconds the drawer takes to slide in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityDetail", meta = (ClampMin = "0", ClampMax = "2"))
	float SlideDuration = .18f;

private:
	/** Moves the drawer from its right edge towards its place, easing out; stops once there. */
	EActiveTimerReturnType Slide(double CurrentTime, float DeltaTime);

	double SlideStartTime = 0.0;
};
