// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/GeoInputConfig.h"

UInputAction const* UGeoInputConfig::FindAbilityInputActionForTag(FGameplayTag const& inputTag, bool bLogNotFound) const
{
	for (FGeoInputAction const& action : AbilityInputActions)
	{
		if (action.InputTag.MatchesTagExact(inputTag))
		{
			if (bLogNotFound && !action.InputAction)
			{
				UE_LOG(LogTemp, Error, TEXT("InputAction for tag [%s] is not valid"), *inputTag.ToString());
			}
			return action.InputAction;
		}
	}
	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find tag [%s] in AbilityInputActions on config [%s]"), *inputTag.ToString(),
			   *GetName());
	}
	return nullptr;
}
