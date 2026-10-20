// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/Cat/GeoCatRoamAction.h"

#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UGeoCatRoamAction::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (!Settings.Area)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("%s: no roam Area set"), *GetName())));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
