// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/Component/GeoDeploySatelliteComponent.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerClassTypes.h"

#include "PlayerClassDataAsset.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UAnimMontage;
class UMaterialInterface;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FPlayerClassData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> AliveMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> DeathMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UAnimInstance> AnimClass;

	/** Played when the character goes down and stopped on revive, so it holds for the whole downed state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	/** Takes DeathMontage's place when the character dies by dropping into the void (the hex arena's holes). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FallMontage = nullptr;

	/** Played when the character gets back up, and the length of the revive it belongs to. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ReviveMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	/** Look of the deploy-charge satellites orbiting this class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FSatelliteParams SatelliteParams;
};

/** Data asset that catalogs mesh/material/animation/attribute data for every player class. Referenced by
 * UGameDataSettings so no player class visuals are baked into APlayableCharacter itself. */
UCLASS()
class GEOTRINITY_API UPlayerClassDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Class")
	TMap<EPlayerClass, FPlayerClassData> ClassData;

	/** Returns Class's authored data, or null with an ensure — a class the map has no entry for is a configuration
	 * bug. The single answer to "what is this character's class data?", so every caller fails the same way. */
	FPlayerClassData const* GetClassData(EPlayerClass Class) const;
};
