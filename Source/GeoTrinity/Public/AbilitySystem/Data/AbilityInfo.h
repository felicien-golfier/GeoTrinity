// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "AbilityInfo.generated.h"


class UGameplayAbility;
class UInputAction;

/** Metadata for a single gameplay ability: its class, auto-populated tag, display name, and description. */
USTRUCT(BlueprintType)
struct FGameplayAbilityInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** Populated automatically from the ability CDO's AssetTags on load — do not set manually */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	FString AbilityDisplayName{"No name set"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	FString Description;
};

/** Extends FGameplayAbilityInfo with player-specific fields: input mapping, startup grant flag, and ability icon. */
USTRUCT(BlueprintType)
struct FPlayersGameplayAbilityInfo : public FGameplayAbilityInfo
{
	GENERATED_BODY()

	/** Populated automatically from the ability CDO's AssetTags on load — do not set manually */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, meta = (Categories = "Ability.Type"))
	FGameplayTag TypeOfAbilityTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InputAction;

	/** If true, this ability is automatically given to players of the matching PlayerClass at startup */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	bool bGiveAtStartup = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	TObjectPtr<UTexture2D const> AbilityIcon{nullptr};
};

/**
 * Data asset that maps ability tags to their classes and metadata, organized by player class.
 * Referenced globally via UGameDataSettings. Abilities are looked up by tag when granting startup abilities
 * or activating abilities via input tag.
 */
UCLASS()
class GEOTRINITY_API UAbilityInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Triangle", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> TriangleAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Circle", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> CircleAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Square", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> SquareAbilities;

	/** Abilities granted to all player classes */
	UPROPERTY(EditDefaultsOnly, Category = "Shared (All Classes)", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> SharedAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Information", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FGameplayAbilityInfo> GenericAbilityInfos;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Information", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FGameplayAbilityInfo> PlayersAbilityInfos;

	/** Calls PopulateAbilityTags() to fill transient AbilityTag fields from CDO asset tags. */
	virtual void PostLoad() override;
#if WITH_EDITOR
	/** Re-populates ability tags when the asset is edited in the editor. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Returns abilities for the given class combined with SharedAbilities. */
	TArray<FPlayersGameplayAbilityInfo> GetAbilitiesForClass(EPlayerClass PlayerClass) const;
	/** Returns all player ability infos across all classes (Triangle + Circle + Square + Shared). */
	TArray<FPlayersGameplayAbilityInfo> GetAllPlayersAbilityInfos() const;

	/** Returns raw pointers to all FGameplayAbilityInfo entries (generic and player infos). */
	TArray<FGameplayAbilityInfo*> GetAllAbilityInfos() const;
	/**
	 * Finds ability infos whose AbilityTag matches any tag in AbilityTags.
	 *
	 * @param AbilityTags       Tags to match against.
	 * @param bLogIfNotFound    When true, logs a warning for each tag with no matching entry.
	 * @return                  Array of matching FGameplayAbilityInfo copies.
	 */
	TArray<FGameplayAbilityInfo> FindAbilityInfoForListOfTag(TArray<FGameplayTag> const& AbilityTags,
															 bool bLogIfNotFound = false) const;

private:
	void PopulateAbilityTags();
};
