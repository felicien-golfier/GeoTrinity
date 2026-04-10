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

/**
 * Base actor for all in-world objects that participate in GAS (turrets, healing zones, buff pickups, etc.).
 * Owns an ASC and attribute set, implements the team interface for attitude queries, and provides a data-init
 * contract (InitInteractableData) so the spawner can configure the actor before BeginPlay.
 */
UCLASS()
class GEOTRINITY_API AGeoInteractableActor
	: public AActor
	, public IAbilitySystemInterface
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGeoInteractableActor();

	// IAbilitySystemInterface BEGIN
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// IAbilitySystemInterface END

	// IGenericTeamAgentInterface BEGIN
	virtual FGenericTeamId GetGenericTeamId() const override;
	// IGenericTeamAgentInterface END

	/**
	 * Called by the spawner before BeginPlay to supply runtime configuration (owner, level, team, seed).
	 * Subclasses must cast Data to their own FDeployableData-derived struct.
	 *
	 * @param Data  Pointer to the configuration struct. Must not be null. Ownership remains with the caller.
	 */
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
