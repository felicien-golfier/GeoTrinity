// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Characters/GeoCharacter.h"
#include "CoreMinimal.h"

#include "EnemyCharacter.generated.h"

/**
 *
 */
UCLASS()
class GEOTRINITY_API AEnemyCharacter : public AGeoCharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter(const FObjectInitializer& ObjectInitializer);
};
