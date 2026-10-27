// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoCatLineFromFileAction.h"

#include "Misc/DataValidation.h"
#include "Tool/CatLine/GeoCatLineBank.h"

#if WITH_EDITOR
EDataValidationResult UGeoCatLineFromFileAction::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (GeoCatLines::CountLines(Categories) == 0)
	{
		Context.AddWarning(
			FText::FromString(FString::Printf(TEXT("%s: no line in the picked categories"), *GetName())));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

FText UGeoCatLineFromFileAction::GetLineText() const
{
	return FText::FromString(GeoCatLines::PickLine(Categories, bRandom));
}
