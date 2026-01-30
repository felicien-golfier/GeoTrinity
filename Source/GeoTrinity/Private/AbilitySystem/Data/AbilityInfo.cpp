// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/AbilityInfo.h"

#include "GeoTrinity/GeoTrinity.h"

FGameplayAbilityInfo UAbilityInfo::FindAbilityInfoForTag(FGameplayTag const& AbilityTag,
														 bool const bLogIfNotFound) const
{
	for (FGameplayAbilityInfo const& Info : AbilityInfos)
	{
		if (Info.AbilityTag == AbilityTag)
		{
			return Info;
		}
	}

	if (bLogIfNotFound)
	{
		UE_LOG(LogGeoASC, Error, TEXT("Tag %s was not found on AbilityInfos %s"), *AbilityTag.GetTagName().ToString(),
			   *GetName());
	}

	return FGameplayAbilityInfo();
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGameplayAbilityInfo> UAbilityInfo::FindAbilityInfoForListOfTag(TArray<FGameplayTag> const& AbilityTags,
																	   bool bLogIfNotFound) const
{
	TArray<FGameplayAbilityInfo> CorrespondingInfos;
	for (FGameplayAbilityInfo const& Info : AbilityInfos)
	{
		if (AbilityTags.Contains(Info.AbilityTag))
		{
			CorrespondingInfos.Add(Info);
		}
	}

	if (bLogIfNotFound && CorrespondingInfos.Num() != AbilityTags.Num())
	{

		UE_LOG(LogGeoASC, Error, TEXT("NOT all tags were found on AbilityInfos %s"), *GetName());
	}

	return CorrespondingInfos;
}
