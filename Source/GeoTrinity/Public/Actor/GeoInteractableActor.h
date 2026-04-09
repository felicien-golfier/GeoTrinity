// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"

#include "GeoInteractableActor.generated.h"

class UGeoGameFeelComponent;

USTRUCT()
struct FInteractableActorData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> CharacterOwner = nullptr;

	UPROPERTY()
	int Level = 1;

	UPROPERTY()
	FGenericTeamId TeamID;

	UPROPERTY()
	int Seed = 0;
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
	virtual FGenericTeamId GetGenericTeamId() const override;
	// IGenericTeamAgentInterface END

	virtual void InitInteractableData(FInteractableActorData* Data);

protected:
	virtual FInteractableActorData const* GetData() const { return nullptr; }

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;

	virtual void InitGas();
	virtual void BindGasCallbacks();
	virtual void UnbindGasCallbacks();
	UFUNCTION(BlueprintNativeEvent)
	void OnHealthChanged(float NewValue);
	virtual void OnHealthChanged_Implementation(float NewValue);
	UFUNCTION(BlueprintNativeEvent)
	void OnMaxHealthChanged(float NewValue);
	virtual void OnMaxHealthChanged_Implementation(float NewValue);

private:
	UFUNCTION(Server, Reliable)
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level);

	UPROPERTY(Category = "GAS", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UGeoAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY(Category = "GAS", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UGeoAttributeSetBase> AttributeSetBase;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameFeel")
	TObjectPtr<UGeoGameFeelComponent> GameFeelComponent;
};
