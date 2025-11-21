// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Lib/GeoGameplayTags.h"

#include "GameplayTagsManager.h"


FGeoGameplayTags FGeoGameplayTags::GameplayTags;

namespace
{
	void CreateAndAssignGameplayTag(FGameplayTag& outTag, FName const& tagName, FString const& comment)
	{
		outTag = UGameplayTagsManager::Get().AddNativeGameplayTag(tagName, comment);
	}

	// INPUT //
	void AddInputTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::InputTag + "." + tagName), comment);
	}
}

void FGeoGameplayTags::InitializeNativeGameplayTags()
{
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Damage			, "Gameplay.Damage", "Tag to identify damage");
	
	// INPUT //
	AddInputTag(GameplayTags.InputTag_BasicSpell		, "BasicSpell", "Input tag for left mouse button-linked spell");
}