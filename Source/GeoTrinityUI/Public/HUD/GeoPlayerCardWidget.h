// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "HUD/GenericCombattantWidget.h"

#include "GeoPlayerCardWidget.generated.h"

class AGeoPlayerState;
class UGeoMeter;
class UGeoShape;
class UTextBlock;

/**
 * One player as the HUD shows them: their class shape and colour, their name and role, a health meter with the shield
 * as its overhang, and — each optional — the health number, the health over the max, the shield line and the class
 * gauge: a frame in the class's shape round it, filling towards the class's big move and pulsing once it is ready. The local player's wing (WBP_PlayerWing) and every team-list row (WBP_TeamRow) are this widget in two
 * layouts. Follows the player's class as it changes.
 */
UCLASS()
class GEOTRINITYUI_API UGeoPlayerCardWidget : public UGenericCombattantWidget
{
	GENERATED_BODY()

public:
	/** Shows InPlayerState's player and binds to their ability system. */
	void InitForPlayer(AGeoPlayerState* InPlayerState);

	AGeoPlayerState* GetPlayerState() const { return PlayerState.Get(); }

protected:
	/** Follows a class change and the class gauge, which no attribute event carries. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;
	/** Adds the health over the max and the shield line to the health the base shows. */
	virtual void RefreshStats() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoShape> ClassShape;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** The class's role in the team: DPS, HEAL, TANK. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoleText;

	/** The health over the max, as HealthFormat writes it. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	/** Collapsed while the player has no shield. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText;

	/** The class gauge as a frame in the class's shape around ClassShape; collapsed for a class without one. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMeter> GaugeRing;

	/** The class gauge as a percentage; collapsed for a class without one. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GaugeText;

	/** {0} is the health, {1} the max health. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoPlayerCard")
	FText HealthFormat = INVTEXT("{0} / {1}");

	/** {0} is the shield. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoPlayerCard")
	FText ShieldFormat = INVTEXT("\u00B7  SHIELD {0}");

	/** {0} is the gauge's whole percentage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoPlayerCard")
	FText GaugeFormat = INVTEXT("\u00B7  CHARGE {0}%");

	/** Sacrificed damage that fills the Square's gauge and readies it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoPlayerCard", meta = (ClampMin = "1"))
	float SacrificeForFullGauge = 200.f;

private:
	/** Shape, colours and role of PlayerClass, from the theme. */
	void ShowClass(EPlayerClass PlayerClass);
	/** Fills the gauge frame and text, or collapses them when the class has no gauge. */
	void RefreshGauge();

	/**
	 * The class gauge of the shown player: the Circle's sweet-spot charge, the Square's armed sacrifice over
	 * SacrificeForFullGauge, ready once full; the Triangle's has no fill and is ready (and full) while a blinking turret
	 * can be recalled. False for a class without one.
	 */
	bool GetClassGauge(float& OutFill, bool& bOutReady) const;

	TWeakObjectPtr<AGeoPlayerState> PlayerState;

	/** Class last shown; unset until the first ShowClass. */
	TOptional<EPlayerClass> ShownClass;
};
