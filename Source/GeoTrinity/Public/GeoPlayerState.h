// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GeoPlayerState.generated.h"

class UGeoAbilitySystemComponent;
/**
 * Deriving just to set up basic RPG stuff for now (felt awkward to put all of this in the controller)
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGeoPlayerState();
	
	/** Implement IAbilitySystemInterface */
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** END Implement IAbilitySystemInterface */
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;
};
