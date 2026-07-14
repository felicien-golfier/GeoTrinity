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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	FString AbilityDisplayName{"No name set"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	FString Description;

	/**
	 * Returns the description text with every {Token} replaced by the matching live value from the ability CDO.
	 * The text comes from the [AbilityTag] section of Content/Data/AbilityDescriptions.txt when present
	 * (editable plain-text file, re-read on every call), falling back to the Description field otherwise.
	 * Supported tokens: Cooldown, FireDelay, Damage/Heal/Shield (summed from the ability's effect data),
	 * Effects (one line per effect entry: damage/heal/shield, buffs with magnitude and duration, statuses
	 * with chance), or the name of any numeric / FScalableFloat property on the ability class.
	 * Scalable values are evaluated at AbilityLevel; a {Token:range} suffix renders them as a "min-max" range
	 * over curve levels 1-10 instead (for values driven by another system than ability level, e.g. the reload's
	 * remaining ammo). A {Token:%} / {Token:+%} suffix formats the scalar as a percentage / bonus percentage;
	 * suffixes combine in any order. With bRichTextValues, every resolved value is wrapped in a <Value>...</>
	 * rich-text style tag so the UI can color it. Unresolved tokens are kept as-is and logged.
	 */
	GEOTRINITY_API FString GetResolvedDescription(int32 AbilityLevel = 1, bool bRichTextValues = false) const;
};

/** Extends FGameplayAbilityInfo with player-specific data: input binding, class filter, and cosmetic icon. */
USTRUCT(BlueprintType)
struct FPlayersGameplayAbilityInfo : public FGameplayAbilityInfo
{
	GENERATED_BODY()

	/** Populated automatically from the ability CDO's AssetTags on load — do not set manually */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (Categories = "Ability.Type"))
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

	/** When true, the ability bar slot shows a remaining-deployable count badge (Wall/Turret abilities). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	bool bShowDeployCount = false;
};

/**
 * Data asset that catalogs all abilities in the game.
 * Three arrays are separated by player class (Triangle/Circle/Square) for input binding and class-specific granting.
 * SharedAbilities are granted to all classes. EnemyAbilityInfos and GetAllPlayersAbilityInfos() serve as flat
 * look-up tables for code that needs to find an ability by tag without caring which class owns it.
 *
 * When to use each container:
 *   - TriangleAbilities / CircleAbilities / SquareAbilities: abilities that belong to exactly one class and
 *     need an InputAction bound. Granted automatically at class startup.
 *   - SharedAbilities: abilities common to all classes (e.g. Dash). Granted to every player at startup.
 *   - GetAllPlayersAbilityInfos(): flat merged view (Triangle + Circle + Square + Shared) for code that needs
 *     to resolve an ability tag without knowing the player's class.
 *   - EnemyAbilityInfos: non-player abilities (enemies, passives, system). No InputAction, no class filter.
 *     Used by UGeoAbilitySystemLibrary::GetAbilityCDO and similar helpers for tag-based look-up.
 */
UCLASS()
class GEOTRINITY_API UAbilityInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> TriangleAbilities;

	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> CircleAbilities;

	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> SquareAbilities;

	/** Abilities granted to all player classes */
	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "{AbilityTag}"))
	TArray<FPlayersGameplayAbilityInfo> SharedAbilities;

	/** Non-player abilities (enemies, passives, system). Used for tag-based CDO look-up. Not bound to any input or
	 * class. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability Information", meta = (TitleProperty = "{AbilityTag}"))
	TArray<FGameplayAbilityInfo> EnemyAbilityInfos;

	/** Re-reads all ability CDO asset tags and syncs description text from the plain-text file after loading. */
	virtual void PostLoad() override;
#if WITH_EDITOR
	/** Writes the changed Description entry back to the plain-text file whenever a Description property is edited in the Details panel. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Returns abilities for the given class combined with SharedAbilities. */
	TArray<FPlayersGameplayAbilityInfo> GetAbilitiesForClass(EPlayerClass PlayerClass) const;
	/** Returns all player ability infos across all classes (Triangle + Circle + Square + Shared). */
	TArray<FPlayersGameplayAbilityInfo> GetAllPlayersAbilityInfos() const;

	/** Returns copies of all FGameplayAbilityInfo entries (generic and player infos). */
	TArray<FGameplayAbilityInfo> GetAllAbilityInfos() const;

	/**
	 * Returns the ability class registered for AbilityTag, or nullptr if the tag is invalid or unknown.
	 * O(1) after the first call: lazily builds and caches a tag->class map. Silent on miss (no assert) — safe to call
	 * on every effect application, including for non-ability sources that pass an invalid tag.
	 */
	TSubclassOf<UGameplayAbility> GetAbilityClassForTag(FGameplayTag AbilityTag) const;
	/**
	 * Finds ability infos whose AbilityTag matches any tag in AbilityTags.
	 *
	 * @param AbilityTags       Tags to match against.
	 * @param bLogIfNotFound    When true, logs a warning for each tag with no matching entry.
	 * @return                  Array of matching FGameplayAbilityInfo copies.
	 */
	TArray<FGameplayAbilityInfo> FindAbilityInfoForListOfTag(TArray<FGameplayTag> const& AbilityTags,
															 bool bLogIfNotFound = false) const;

	/** Re-reads every ability CDO's AssetTags and fills AbilityTag fields. Call from Python after modifying ability arrays. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	void PopulateAbilityTags();

private:
	/** Pointers to every entry across all arrays, viewed as the shared base struct. */
	TArray<FGameplayAbilityInfo*> GetAllAbilityInfoPtrs();

#if WITH_EDITOR
	/** Resolves the entry at Index in the array named ArrayName (one of the ability arrays), or nullptr. */
	FGameplayAbilityInfo* FindAbilityInfo(FName ArrayName, int32 Index);
#endif

	// Lazily-built tag->class map backing GetAbilityClassForTag. Cleared in PostLoad/PopulateAbilityTags so it rebuilds
	// after the ability arrays or their tags change. Mutable: GetAbilityClassForTag is logically const.
	mutable TMap<FGameplayTag, TSubclassOf<UGameplayAbility>> AbilityClassByTag;
};
