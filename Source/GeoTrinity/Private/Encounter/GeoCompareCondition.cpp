// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoCompareCondition.h"

#include "Encounter/GeoEncounter.h"

bool UGeoCompareCondition::Evaluate_Implementation(FGeoEncounterPayload const& Payload) const
{
	double Left = Payload.Count;
	if (Source == EGeoCompareSource::PayloadValue)
	{
		Left = Payload.Value;
	}
	else if (Source == EGeoCompareSource::Counter)
	{
		AGeoEncounter const* const Encounter = GetEncounter();
		if (ensureMsgf(Encounter, TEXT("Condition %s used outside an encounter"), *GetName()))
		{
			Left = Encounter->GetCounter(CounterName);
		}
	}

	switch (Comparison)
	{
	case EGeoCompareOperator::Equal:
		return FMath::IsNearlyEqual(Left, static_cast<double>(Operand));
	case EGeoCompareOperator::NotEqual:
		return !FMath::IsNearlyEqual(Left, static_cast<double>(Operand));
	case EGeoCompareOperator::Less:
		return Left < Operand;
	case EGeoCompareOperator::LessOrEqual:
		return Left <= Operand;
	case EGeoCompareOperator::Greater:
		return Left > Operand;
	case EGeoCompareOperator::GreaterOrEqual:
		return Left >= Operand;
	case EGeoCompareOperator::MultipleOf:
		return Operand != 0.f && FMath::IsNearlyZero(FMath::Fmod(Left, static_cast<double>(Operand)));
	}
	return false;
}
