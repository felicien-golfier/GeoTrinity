// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "AbilityInfo.generated.h"


class UGameplayAbility;

USTRUCT(BlueprintType)
struct FGameplayAbilityInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay", meta = (Categories = "Ability.Cooldown"))
	FGameplayTag CooldownTag{};

	/** This is deduced from the ability obtained from the tag */
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
	FGameplayTag InputTag{};

	/** This is deduced from the ability obtained from the tag */
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
	FGameplayTag StatusTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay", meta = (Categories = "Ability.Type"))
	FGameplayTag TypeOfAbilityTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	EPlayerClass PlayerClass = EPlayerClass::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	TObjectPtr<UTexture2D const> AbilityIcon{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	FString AbilityDisplayName{"No name set"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Gameplay", meta = (Units = Seconds))
	float AbilityCooldownDuration{0.f};

	UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
	float ManaCost{0.f};
};

/**
 *
 */
UCLASS()
class GEOTRINITY_API UAbilityInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Ability Information", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FGameplayAbilityInfo> AbilityInfos;

	UPROPERTY(EditDefaultsOnly, Category = "Input Information",
			  meta = (TitleProperty = "{Key}", ForceInlineRow, KeyCategories = "Ability.Type",
					  ValueCategories = "InputTag"))
	TMap<FGameplayTag, FGameplayTag> AbilityTypeToInputTagMap;

	FGameplayAbilityInfo FindAbilityInfoForTag(FGameplayTag const& AbilityTag, bool bLogIfNotFound = false) const;
	TArray<FGameplayAbilityInfo> FindAbilityInfoForListOfTag(TArray<FGameplayTag> const& AbilityTags,
															 bool bLogIfNotFound = false) const;
};
