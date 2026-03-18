// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/AbilityInfo.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GameplayTagsManager.h"
#include "GeoTrinity/GeoTrinity.h"

static FGameplayTag GetAbilityTagFromCDO(TSubclassOf<UGameplayAbility> const& AbilityClass)
{
	if (!AbilityClass)
	{
		return FGameplayTag{};
	}

	FGameplayTag const SpellRoot =
		UGameplayTagsManager::Get().RequestGameplayTag(FName(RootTagNames::AbilityIDTag), false);
	for (FGameplayTag const& Tag : AbilityClass.GetDefaultObject()->GetAssetTags())
	{
		if (Tag.MatchesTag(SpellRoot))
		{
			return Tag;
		}
	}

	ensureMsgf(false, TEXT("Ability %s has no AbilityTag, please fill the AssetTags in the BP under Tag category"),
			   *AbilityClass->GetName());
	return FGameplayTag{};
}

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PopulateAbilityTags()
{
	for (FGameplayAbilityInfo& Info : GenericAbilityInfos)
	{
		Info.AbilityTag = GetAbilityTagFromCDO(Info.AbilityClass);
	}
	for (FPlayersGameplayAbilityInfo& Info : PlayersAbilityInfos)
	{
		Info.AbilityTag = GetAbilityTagFromCDO(Info.AbilityClass);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PostLoad()
{
	Super::PostLoad();
	PopulateAbilityTags();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	PopulateAbilityTags();
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGameplayAbilityInfo*> UAbilityInfo::GetAllAbilityInfos() const
{
	TArray<FGameplayAbilityInfo*> AllInfos;
	for (auto PlayersAbilityInfo : PlayersAbilityInfos)
	{
		AllInfos.Add(&PlayersAbilityInfo);
	}

	for (auto GenericAbilityInfo : GenericAbilityInfos)
	{
		AllInfos.Add(&GenericAbilityInfo);
	}

	return AllInfos;
}
// ---------------------------------------------------------------------------------------------------------------------
TArray<FGameplayAbilityInfo> UAbilityInfo::FindAbilityInfoForListOfTag(TArray<FGameplayTag> const& AbilityTags,
																	   bool bLogIfNotFound) const
{
	TArray<FGameplayAbilityInfo> CorrespondingInfos;
	for (FGameplayAbilityInfo const& Info : GenericAbilityInfos)
	{
		if (AbilityTags.Contains(Info.AbilityTag))
		{
			CorrespondingInfos.Add(Info);
		}
	}

	for (FGameplayAbilityInfo const& Info : PlayersAbilityInfos)
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
