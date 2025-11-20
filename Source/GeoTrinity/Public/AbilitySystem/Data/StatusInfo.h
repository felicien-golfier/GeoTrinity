// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Engine/DataAsset.h"
#include "StatusInfo.generated.h"


class UGameplayEffect;
/**
 * How to define a status gameplay-wise (a debuff)
 */
USTRUCT(BlueprintType)
struct FRpgStatusInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag StatusTag {};
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> StatusEffect {nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	bool bDoesDamage {false};
	
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta=(EditCondition = "bDoesDamage", EditConditionHides))
	FScalableFloat DamageAmount {0.f};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FString StatusDisplayName {"No name set"};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FString Description;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FSlateBrush Icon;
};

/**
 * All info about gameplay statuses in the game
 */
UCLASS()
class GEOTRINITY_API UStatusInfo : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(TitleProperty="{StatusDisplayName}"))
	TArray<FRpgStatusInfo> StatusInfos;

	bool FillStatusInfoFromTag(FGameplayTag const& tag, FRpgStatusInfo& outInfo) const;
};
