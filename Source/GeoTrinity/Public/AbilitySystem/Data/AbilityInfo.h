// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "AbilityInfo.generated.h"


class UGameplayAbility;
class UInputAction;

/** Base ability descriptor shared by both player-facing and generic (non-player) abilities. */
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

/** Extends FGameplayAbilityInfo with player-specific data: input binding, class filter, and cosmetic icon. */
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
 * Data asset that catalogs all abilities in the game.
 * Three arrays are separated by player class (Triangle/Circle/Square) for input binding and class-specific granting.
 * SharedAbilities are granted to all classes. GenericAbilityInfos and PlayersAbilityInfos serve as flat look-up
 * tables for code that needs to find an ability by tag without caring which class owns it.
 *
 * When to use each container:
 *   - TriangleAbilities / CircleAbilities / SquareAbilities: abilities that belong to exactly one class and
 *     need an InputAction bound. Granted automatically at class startup.
 *   - SharedAbilities: abilities common to all classes (e.g. Dash). Granted to every player at startup.
 *   - PlayersAbilityInfos: flat merged view used by code that needs to resolve an ability tag without knowing
 *     the player's class. Populated from the per-class + shared arrays via GetAllPlayersAbilityInfos().
 *   - GenericAbilityInfos: non-player abilities (enemy, passive, system). No InputAction, no class filter.
 *     Used by UGeoAbilitySystemLibrary::GetAbilityCDO and similar helpers for tag-based look-up.
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

	/** Non-player abilities (enemies, passives, system). Used for tag-based CDO look-up. Not bound to any input or class. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability Information", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FGameplayAbilityInfo> GenericAbilityInfos;

	/** Flat look-up table combining all player abilities across all classes. Populated manually from the per-class arrays.
	 * Use GetAllPlayersAbilityInfos() to get the authoritative runtime-merged list instead. */
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

	/** Returns copies of all FGameplayAbilityInfo entries (generic and player infos). */
	TArray<FGameplayAbilityInfo> GetAllAbilityInfos() const;
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
