// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
class GEOTRINITY_API AGeoTurretBase : public AActor
{
	GENERATED_BODY()

public:
	AGeoTurretBase();
	void InitTurretData(TurretInitData const& data);
	
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(EditAnywhere, Category = GAS)
	TSubclassOf<UGameplayEffect> DefaultAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GAS)
	TArray<TSubclassOf<UGeoGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GAS)
	TArray<FGameplayTag> StartupAbilityTags;
	
private:
	void InitializeDefaultAttributes();
	void AddDefaultAbilities();
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);
	
	UPROPERTY()
	TObjectPtr<UGeoAbilitySystemComponent> ASC;
	
	UPROPERTY()
	TObjectPtr<UGeoAttributeSetBase> AttributeSet;

	UPROPERTY()
	TWeakObjectPtr<AActor> CharacterOwner;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	float TurretLevel {1.f};
	FDamageEffectParams BulletsDamageEffectParams;
};