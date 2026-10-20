// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayableCharacter.h"
#include "CoreMinimal.h"
#include "HUD/GeoHUD.h"
#include "HUD/GeoUserWidget.h"

#include "GeoAbilityBarWidget.generated.h"

class UHorizontalBox;
class UWrapBox;
class UGeoAbilitySlotWidget;
class AGeoHUD;
class UGeoAbilitySystemComponent;

/**
 * Bottom-center ability bar. On BuildBar it clears SlotBox, queries AGeoHUD::GetAbilityBarEntries, and creates one
 * WBP_AbilitySlot per entry. Binds the HUD's deploy-count ping so deployable slots refresh their badges without
 * polling. Passives get a tile each in PassiveBox. A grant or removal on the player's ASC (OnAbilitySetChanged) queues
 * one rebuild for the next tick, so a burst of grants costs one.
 */
UCLASS()
class GEOTRINITYUI_API UGeoAbilityBarWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuilds the bar from the HUD's current ability set. Called via UGeoOverlayWidget::BuildAbilityBar. */
	UFUNCTION(BlueprintCallable, Category = "GeoAbilityBar")
	void BuildBar(AGeoHUD* InHUD, APlayableCharacter* PlayableCharacter);

protected:
	/** Unbinds the ASC grant event. */
	virtual void NativeDestruct() override;

	/** Re-broadcast target: re-queries every deployable slot's count on the HUD's tagless ping. */
	UFUNCTION()
	void OnDeployCountChanged();

	/** Horizontal container the slots are added to. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> SlotBox;

	/** Vertical wrap box left of SlotBox; its wrap size is two tiles, so passives stack two per column. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> PassiveBox;

	/** Slot widget class to instantiate per ability (set to WBP_AbilitySlot in the bar BP). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbilityBar")
	TSubclassOf<UGeoAbilitySlotWidget> SlotWidgetClass;

	/** Slot widget class to instantiate per passive (set to WBP_PassiveSlot in the bar BP). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbilityBar")
	TSubclassOf<UGeoAbilitySlotWidget> PassiveSlotWidgetClass;

private:
	/** ASC grant event: queues one BuildBar for the next tick. */
	void OnAbilitySetChanged();

	/** Rebuilds the bar from the HUD's current ability set. */
	void Rebuild();

	/** Replaces PassiveBox's tiles with one per entry of Passives. */
	void BuildPassiveTiles(TArray<FGeoAbilityBarEntry> const& Passives);

	UPROPERTY()
	TObjectPtr<AGeoHUD> HUD;

	UPROPERTY()
	TObjectPtr<APlayableCharacter> Character;

	UPROPERTY()
	TArray<TObjectPtr<UGeoAbilitySlotWidget>> Slots;

	/** The ASC whose grants this bar follows. */
	TWeakObjectPtr<UGeoAbilitySystemComponent> BoundASC;

	FDelegateHandle AbilitySetHandle;
	bool bRebuildQueued = false;
};
