// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoAbilityBarWidget.generated.h"

class UHorizontalBox;
class UGeoAbilitySlotWidget;
class AGeoHUD;

/**
 * Bottom-center ability bar. On BuildBar it clears SlotBox, queries AGeoHUD::GetAbilityBarEntries, and creates one
 * WBP_AbilitySlot per entry. Binds the HUD's deploy-count ping so deployable slots refresh their badges without polling.
 */
UCLASS()
class GEOTRINITY_API UGeoAbilityBarWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuilds the bar from the HUD's current ability set. Called via UGeoOverlayWidget::BuildAbilityBar. */
	UFUNCTION(BlueprintCallable, Category = "AbilityBar")
	void BuildBar(AGeoHUD* InHUD);

protected:
	/** Re-broadcast target: re-queries every deployable slot's count on the HUD's tagless ping. */
	UFUNCTION()
	void OnDeployCountChanged();

	/** Horizontal container the slots are added to. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> SlotBox;

	/** Slot widget class to instantiate per ability (set to WBP_AbilitySlot in the bar BP). */
	UPROPERTY(EditDefaultsOnly, Category = "AbilityBar")
	TSubclassOf<UGeoAbilitySlotWidget> SlotWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<AGeoHUD> HUD;

	UPROPERTY()
	TArray<TObjectPtr<UGeoAbilitySlotWidget>> Slots;
};
