// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "GameFramework/Actor.h"
#include "GeoTurretBase.generated.h"

class UGeoAttributeSetBase;
class UCapsuleComponent;
class UGeoGameplayAbility;
class UGameplayEffect;
class UGeoAbilitySystemComponent;

struct TurretInitData
{
	AActor* CharacterOwner;
	
	float TurretLevel {1.f};
	
	FDamageEffectParams BulletsDamageEffectParams;
};



UCLASS()
class GEOTRINITY_API AGeoTurretBase : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGeoTurretBase();
	void InitTurretData(TurretInitData const& data);
	
	virtual void BeginPlay() override;
	
	// IAbilitySystemInterface BEGIN
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// IAbilitySystemInterface END
	
private:
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	TurretInitData TurretData;
};