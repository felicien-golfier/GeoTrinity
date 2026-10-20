// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterCondition.h"

#include "GeoCompareCondition.generated.h"

/** Passes when the payload Count, the payload Value or a step counter, compared with Operand, holds. */
UCLASS()
class GEOTRINITY_API UGeoCompareCondition : public UGeoEncounterCondition
{
	GENERATED_BODY()

public:
	virtual bool Evaluate_Implementation(FGeoEncounterPayload const& Payload) const override;

	/** True when the source is a counter and no counter name is set. */
	bool IsCounterNameMissing() const { return Source == EGeoCompareSource::Counter && CounterName.IsNone(); }

	UPROPERTY(EditAnywhere, Category = "GeoEncounter")
	EGeoCompareSource Source = EGeoCompareSource::PayloadCount;

	UPROPERTY(EditAnywhere, Category = "GeoEncounter",
			  meta = (EditCondition = "Source == EGeoCompareSource::Counter", EditConditionHides))
	FName CounterName;

	UPROPERTY(EditAnywhere, Category = "GeoEncounter")
	EGeoCompareOperator Comparison = EGeoCompareOperator::Equal;

	/** For MultipleOf: the divisor (0 never passes). */
	UPROPERTY(EditAnywhere, Category = "GeoEncounter")
	float Operand = 0.f;
};
