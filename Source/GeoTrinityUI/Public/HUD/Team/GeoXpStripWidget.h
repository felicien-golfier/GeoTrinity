// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoXpStripWidget.generated.h"

class AGeoPlayerState;
class UGeoGemSubsystem;
class UGeoMeter;
class UTextBlock;

/**
 * The local player's class progress, quiet: a thin bar that fills with the XP into the current level (then, at the max
 * class level, into the current echo level, with no visual difference), the class level and the echo level as bare
 * numbers. The echo slot is Hidden, not Collapsed, while the shared echo level is 0 so the class level and the bar
 * never move when the echo lands. Sits under the health meter in WBP_PlayerWing (WBP_XpStrip).
 */
UCLASS()
class GEOTRINITYUI_API UGeoXpStripWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Follows PlayerState's class and its local player's gem profile, and shows them now. */
	void InitForPlayer(AGeoPlayerState* InPlayerState);

	/** Writes the shown class's level, echo level and XP fill from the local profile; the card calls it on a class
	 * change. */
	void Refresh();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMeter> XpMeter;

	/** The class level, a bare number. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	/** The echo level, a bare number, never with a max. Hidden while the shared echo level is 0. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EchoLevelText;

	/** {0} is the echo level: the middle point keeps it apart from the class level it follows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoXpStrip")
	FText EchoLevelFormat = INVTEXT("\u00B7 {0}");

	/** Colour of the class level number. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoXpStrip")
	FLinearColor LevelColor = FLinearColor(.78f, .70f, .96f, 1.f);

	/** Colour of the echo level number: not the class level's, so the two read apart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoXpStrip")
	FLinearColor EchoLevelColor = FLinearColor(.45f, .2f, 1.f, 1.f);

private:
	TWeakObjectPtr<AGeoPlayerState> PlayerState;

	/** The shown player's local gem subsystem, bound for OnProfileCommitted. */
	TWeakObjectPtr<UGeoGemSubsystem> GemSubsystem;
};
