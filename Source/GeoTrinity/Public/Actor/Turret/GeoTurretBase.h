// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/InteractableComponent.h"
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
	
	FGenericTeamId TeamID;
};



UCLASS()
class GEOTRINITY_API AGeoTurretBase : public AActor, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGeoTurretBase();
	void InitTurretData(TurretInitData const& data);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// IAbilitySystemInterface BEGIN
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// IAbilitySystemInterface END
	
	// IGenericTeamAgentInterface BEGIN
	virtual FGenericTeamId GetGenericTeamId() const override { return TurretData.TeamID; }
	// IGameplayTaskOwnerInterface END
	
protected:
	/* Gas callbacks */
	virtual void BindGasCallbacks();	// this stuff and the rest will go to the common parent
	virtual void UnbindGasCallbacks();
	UFUNCTION()
	virtual void OnHealthChanged(float NewValue);
	UFUNCTION()
	virtual void OnMaxHealthChanged(float NewValue);
	
private:
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);

	void InitAbilityActorInfo();
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	UPROPERTY(Category = "GAS", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY(Category = "GAS", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoAttributeSetBase> AttributeSetBase;
	
	TurretInitData TurretData;
};