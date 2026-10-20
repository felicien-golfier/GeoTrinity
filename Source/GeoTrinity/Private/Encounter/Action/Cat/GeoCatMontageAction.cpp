// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/Cat/GeoCatMontageAction.h"

#include "Actor/GeoCat.h"
#include "Misc/DataValidation.h"

void UGeoCatMontageAction::GiveOrder(AGeoCat& OrderedCat) const
{
	OrderedCat.PlayMontage(Montage);
}

#if WITH_EDITOR
EDataValidationResult UGeoCatMontageAction::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (!Montage)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("%s: no Montage set"), *GetName())));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
