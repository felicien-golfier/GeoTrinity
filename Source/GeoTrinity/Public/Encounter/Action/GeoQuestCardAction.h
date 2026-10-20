// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoQuestCardAction.generated.h"

/** What a GeoQuestCardAction does to the card. */
UENUM(BlueprintType)
enum class EGeoQuestCardOperation : uint8
{
	/** Shows the card at 0 over Max. */
	Show,
	/** Shows the payload's Count, or Max with bShowMax. */
	Count,
	Hide
};

/** Drives the local player's quest card (the tutorial's kill-quest gag): show it, update its count, hide it. */
UCLASS()
class GEOTRINITY_API UGeoQuestCardAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	EGeoQuestCardOperation Operation = EGeoQuestCardOperation::Show;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter",
			  meta = (EditCondition = "Operation == EGeoQuestCardOperation::Show", EditConditionHides))
	FText Title;

	/** The count the quest asks for; also what bShowMax jumps to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "1"))
	int32 Max = 100;

	/** Quest time limit the card counts down from (decoration only, nothing happens at 0); 0 shows none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter",
			  meta = (ClampMin = "0", EditCondition = "Operation == EGeoQuestCardOperation::Show", EditConditionHides))
	float TimeLimitSeconds = 1800.f;

	/** True: the count jumps to Max, whatever the payload says (the quest cut). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter",
			  meta = (EditCondition = "Operation == EGeoQuestCardOperation::Count", EditConditionHides))
	bool bShowMax = false;
};
