// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/Cat/GeoCatAction.h"

#include "Actor/GeoCat.h"
#include "Encounter/GeoEncounter.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Misc/DataValidation.h"

void UGeoCatAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	if (!Cat)
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no Cat set"), *GetName());
		Finish();
	}
	else
	{
		GiveOrder(*Cat);
		if (bWaitForEnd && EndsOnItsOwn() && Cat->IsOrderRunning())
		{
			OrderEndedHandle = Cat->OnOrderEnded.AddUObject(this, &UGeoCatAction::OnOrderEnded);
		}
		else
		{
			Finish();
		}
	}
}

void UGeoCatAction::OnCancel_Implementation()
{
	StopWaiting();
	if (IsValid(Cat) && GetEncounter()->IsAborting())
	{
		Cat->Stop();
	}
}

void UGeoCatAction::OnOrderEnded(bool /*bCompleted*/)
{
	StopWaiting();
	Finish();
}

void UGeoCatAction::StopWaiting()
{
	if (IsValid(Cat))
	{
		Cat->OnOrderEnded.Remove(OrderEndedHandle);
	}
	OrderEndedHandle.Reset();
}

#if WITH_EDITOR
EDataValidationResult UGeoCatAction::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (!Cat)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("%s: no Cat set"), *GetName())));
	}

	if (bWaitForEnd && !EndsOnItsOwn())
	{
		Context.AddWarning(
			FText::FromString(FString::Printf(TEXT("%s: this order never ends, bWaitForEnd is ignored"), *GetName())));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
