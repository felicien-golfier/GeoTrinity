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
		return FGameplayTag();
	}

	FGameplayTag const SpellRoot =
		UGameplayTagsManager::Get().RequestGameplayTag(FName(RootTagNames::AbilitySpellTag), false);
	for (FGameplayTag const& Tag : AbilityClass.GetDefaultObject()->GetAssetTags())
	{
		if (Tag.MatchesTag(SpellRoot))
		{
			return Tag;
		}
	}

	ensureMsgf(false, TEXT("Ability %s has no AbilityTag, please fill the AssetTags in the BP under Tag category"),
			   *AbilityClass->GetName());

	return FGameplayTag();
}

static FGameplayTag GetAbilityTypeTagFromCDO(TSubclassOf<UGameplayAbility> const& AbilityClass)
{
	if (!AbilityClass)
	{
		return FGameplayTag();
	}

	FGameplayTag const AbilityTypeRoot = FGeoGameplayTags::Get().Ability_Type;
	for (FGameplayTag const& Tag : AbilityClass.GetDefaultObject()->GetAssetTags())
	{
		if (Tag.MatchesTag(AbilityTypeRoot))
		{
			return Tag;
		}
	}

	ensureMsgf(false, TEXT("Ability %s has no Ability Type, please fill the AssetTags in the BP under Tag category"),
			   *AbilityClass->GetName());
	return FGameplayTag();
}

static void PopulateTagsForArray(TArray<FPlayersGameplayAbilityInfo>& Infos)
{
	for (FPlayersGameplayAbilityInfo& Info : Infos)
	{
		Info.AbilityTag = GetAbilityTagFromCDO(Info.AbilityClass);
		Info.TypeOfAbilityTag = GetAbilityTypeTagFromCDO(Info.AbilityClass);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PopulateAbilityTags()
{
	for (FGameplayAbilityInfo& Info : GenericAbilityInfos)
	{
		Info.AbilityTag = GetAbilityTagFromCDO(Info.AbilityClass);
	}
	PopulateTagsForArray(TriangleAbilities);
	PopulateTagsForArray(CircleAbilities);
	PopulateTagsForArray(SquareAbilities);
	PopulateTagsForArray(SharedAbilities);
}

// ---------------------------------------------------------------------------------------------------------------------
void UAbilityInfo::PostLoad()
{
	Super::PostLoad();
	if (!FGeoGameplayTags::AreNativeTagsInitialized())
	{
		return;
	}
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
TArray<FPlayersGameplayAbilityInfo> UAbilityInfo::GetAbilitiesForClass(EPlayerClass PlayerClass) const
{
	ensureMsgf(PlayerClass != EPlayerClass::None && PlayerClass != EPlayerClass::All,
			   TEXT("GetAbilitiesForClass called with invalid PlayerClass %s"), *UEnum::GetValueAsString(PlayerClass));

	TArray<FPlayersGameplayAbilityInfo> Result = SharedAbilities;
	switch (PlayerClass)
	{
	case EPlayerClass::Triangle:
		Result.Append(TriangleAbilities);
		break;
	case EPlayerClass::Circle:
		Result.Append(CircleAbilities);
		break;
	case EPlayerClass::Square:
		Result.Append(SquareAbilities);
		break;
	default:
		break;
	}
	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FPlayersGameplayAbilityInfo> UAbilityInfo::GetAllPlayersAbilityInfos() const
{
	TArray<FPlayersGameplayAbilityInfo> Result;
	Result.Append(TriangleAbilities);
	Result.Append(CircleAbilities);
	Result.Append(SquareAbilities);
	Result.Append(SharedAbilities);
	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGameplayAbilityInfo> UAbilityInfo::GetAllAbilityInfos() const
{
	TArray<FGameplayAbilityInfo> AllInfos;
	for (FPlayersGameplayAbilityInfo const& Info : GetAllPlayersAbilityInfos())
	{
		AllInfos.Add(Info);
	}
	for (FGameplayAbilityInfo const& Info : GenericAbilityInfos)
	{
		AllInfos.Add(Info);
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

	for (FPlayersGameplayAbilityInfo const& Info : GetAllPlayersAbilityInfos())
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
