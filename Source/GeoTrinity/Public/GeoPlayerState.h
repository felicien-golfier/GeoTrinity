// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "InputAction.h"

#include "GeoPlayerState.generated.h"

class UGeoAttributeSetBase;
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

	/** Implement IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** END Implement IAbilitySystemInterface */

	UGeoAttributeSetBase* GetGeoAttributeSetBase() const { return AttributeSetBase; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGeoAttributeSetBase> AttributeSetBase;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputAction> MoveAction;
};
