// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"

#include "GeoInteractableActor.generated.h"

struct FInteractableActorData
{
	AActor* CharacterOwner;
	float Level{1.f};
	FGenericTeamId TeamID;
};

UCLASS()
class GEOTRINITY_API AGeoInteractableActor
	: public AActor
	, public IAbilitySystemInterface
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGeoInteractableActor();

	// IAbilitySystemInterface BEGIN
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// IAbilitySystemInterface END

	// IGenericTeamAgentInterface BEGIN
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(); }
	// IGameplayTaskOwnerInterface END

	virtual void InitInteractableData(FInteractableActorData* Data);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;

	virtual void InitGas();
	virtual void BindGasCallbacks();
	virtual void UnbindGasCallbacks();
	UFUNCTION()
	virtual void OnHealthChanged(float NewValue);
	UFUNCTION()
	virtual void OnMaxHealthChanged(float NewValue);

private:
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);

	UPROPERTY(Category = "GAS", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UGeoAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY(Category = "GAS", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UGeoAttributeSetBase> AttributeSetBase;

	FInteractableActorData* InteractableActorData;
};
