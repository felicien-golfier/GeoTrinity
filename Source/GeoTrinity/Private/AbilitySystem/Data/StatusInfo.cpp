// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/StatusInfo.h"

bool UStatusInfo::FillStatusInfoFromTag(FGameplayTag const& tag, FRpgStatusInfo& outInfo) const
{
	FRpgStatusInfo const* pInfo = StatusInfos.FindByPredicate([&tag](FRpgStatusInfo const& info) { return info.StatusTag.MatchesTag(tag); });
	if (!pInfo)
	{
		return false;
	}
	outInfo = *pInfo;
	return true;
}