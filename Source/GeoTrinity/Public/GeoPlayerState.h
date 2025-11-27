// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "InputAction.h"

#include "GeoPlayerState.generated.h"

class UCharacterAttributeSet;
class UGeoAbilitySystemComponent;
/**
 * Deriving just to set up basic RPG stuff for now (felt awkward to put all of this in the controller)
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerState
	: public APlayerState
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGeoPlayerState();
	virtual void BeginPlay() override;
	void InitializeInteractableComponent();

	virtual void ClientInitialize(class AController* Controller) override;
	void InitOverlay();

	UFUNCTION()
	void OnPlayerPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	/** Implement IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** END Implement IAbilitySystemInterface */

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCharacterAttributeSet> AttributeSetBase;
};
