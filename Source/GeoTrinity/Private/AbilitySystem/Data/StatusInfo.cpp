// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/StatusInfo.h"

bool UStatusInfo::FillStatusInfoFromTag(FGameplayTag const& Tag, FRpgStatusInfo& OutInfo) const
{
	FRpgStatusInfo const* FoundInfo = StatusInfos.FindByPredicate(
		[&Tag](FRpgStatusInfo const& Info)
		{
			return Info.StatusTag.MatchesTagExact(Tag);
		});
	if (!FoundInfo)
	{
		return false;
	}
	OutInfo = *FoundInfo;
	return true;
}
