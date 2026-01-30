// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoTurretBase.generated.h"

class UGeoAttributeSetBase;
class UCapsuleComponent;
class UGeoGameplayAbility;
class UGameplayEffect;
class UGeoAbilitySystemComponent;

struct FTurretData : FInteractableActorData
{
	TArray<TInstancedStruct<struct FEffectData>> EffectDataArray;
};

UCLASS()
class GEOTRINITY_API AGeoTurretBase : public AGeoInteractableActor
{
	GENERATED_BODY()

public:
	AGeoTurretBase();

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
};
