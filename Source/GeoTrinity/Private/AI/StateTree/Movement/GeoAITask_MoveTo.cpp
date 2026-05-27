// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/GeoAITask_MoveTo.h"

#include "NavigationData.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAITask_MoveTo::PerformMove()
{
	Super::PerformMove();

	if (Path.IsValid())
	{
		Path->EnableRecalculationOnInvalidation(true);
	}
}
