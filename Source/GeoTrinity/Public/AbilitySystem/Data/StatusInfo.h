// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"

#include "StatusInfo.generated.h"


class UGameplayEffect;
/** Defines one status (debuff/buff) including its gameplay effect, optional periodic damage, and display data. */
USTRUCT(BlueprintType)
struct FRpgStatusInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag StatusTag{};

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> StatusEffect{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	bool bDoesDamage{false};

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (EditCondition = "bDoesDamage", EditConditionHides))
	FScalableFloat DamageAmount{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FString StatusDisplayName{"No name set"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FString Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FSlateBrush Icon;
};

/** Data asset listing all status effects in the game. Used by UGeoAbilitySystemLibrary::ApplyStatusToTarget
 *  to look up the correct GE by tag at runtime. */
UCLASS()
class GEOTRINITY_API UStatusInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "{StatusDisplayName}"))
	TArray<FRpgStatusInfo> StatusInfos;

	/**
	 * Finds the FRpgStatusInfo entry whose StatusTag matches the given tag.
	 *
	 * @param tag      The gameplay tag to search for.
	 * @param outInfo  Populated with the found entry if the function returns true.
	 * @return         True if a matching entry was found.
	 */
	bool FillStatusInfoFromTag(FGameplayTag const& tag, FRpgStatusInfo& outInfo) const;
};
