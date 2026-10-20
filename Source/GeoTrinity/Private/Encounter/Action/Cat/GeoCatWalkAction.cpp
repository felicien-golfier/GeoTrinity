// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/Cat/GeoCatWalkAction.h"

#include "Actor/GeoCat.h"
#include "Misc/DataValidation.h"

void UGeoCatWalkAction::GiveOrder(AGeoCat& OrderedCat) const
{
	TArray<FVector> Points;
	for (AActor const* const Point : Path)
	{
		if (Point)
		{
			Points.Add(Point->GetActorLocation());
		}
	}

	OrderedCat.Walk(Points, Speed, StopDistance);
}

#if WITH_EDITOR
EDataValidationResult UGeoCatWalkAction::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (Path.IsEmpty() || Path.Contains(nullptr))
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("%s: empty Path or a null point"), *GetName())));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
