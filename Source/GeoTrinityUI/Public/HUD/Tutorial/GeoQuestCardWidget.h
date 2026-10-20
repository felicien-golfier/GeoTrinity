// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoQuestCardWidget.generated.h"

class FActiveTimerHandle;
class UTextBlock;
enum class EActiveTimerReturnType : uint8;

/**
 * The tutorial's quest card (WBP_QuestCard, step 3 gag): a title, a count over a maximum and a quest time limit
 * counting down from the card's show, on world time (holds during the solo pause), stopping at 00:00. Shows what the
 * encounter tells it, decides nothing. Required in the BP hierarchy: UTextBlock "TitleText", "CountText"; optional
 * UTextBlock "TimerText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoQuestCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows the card at a count of 0; TimeLimitSeconds 0 hides the countdown. */
	void ShowCard(FText const& Title, int32 InMax, float InTimeLimitSeconds);

	void SetCount(int32 Count);

	void HideCard();

protected:
	/** Starts collapsed. */
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimerText;

	/** {0} is the count, {1} the maximum. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoQuestCard")
	FText CountFormat = INVTEXT("KILLED {0}/{1}");

	/** {0} is the minutes left, {1} the seconds (two digits). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoQuestCard")
	FText TimerFormat = INVTEXT("{0}:{1}");

private:
	EActiveTimerReturnType TickCountdown(double CurrentTime, float DeltaTime);

	int32 Max = 0;
	float TimeLimitSeconds = 0.f;
	/** World time at ShowCard. */
	double ShownTime = 0.;
	TWeakPtr<FActiveTimerHandle> CountdownTimer;
};
