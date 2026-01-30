// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GeoHUD.generated.h"

class UGeoUserWidget;
class UGeoAttributeSetBase;
class AGeoPlayerController;
class UAttributeSet;
class UAbilitySystemComponent;
class UGeoAbilitySystemComponent;
class APlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeModifiedSignature, float, NewValue);


USTRUCT(BlueprintType)
struct FHudPlayerParams
{
	GENERATED_BODY()
	FHudPlayerParams() {}
	FHudPlayerParams(TObjectPtr<APlayerController> const& playerController, TObjectPtr<APlayerState> const& playerState,
					 TObjectPtr<UAbilitySystemComponent> const& abilitySystemComponent,
					 TObjectPtr<UAttributeSet> const& attributeSet) :
		PlayerController(playerController), PlayerState(playerState), AbilitySystemComponent(abilitySystemComponent),
		AttributeSet(attributeSet)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAttributeSet> AttributeSet = nullptr;

	AGeoPlayerController* GetGeoPlayerController() const;
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponent() const;
	UGeoAttributeSetBase* GetGeoAttributeSet() const;
};


/**
 * Base class for a HUD in the game
 */
UCLASS()
class GEOTRINITY_API AGeoHUD : public AHUD
{
	GENERATED_BODY()

public:
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

	FHudPlayerParams const& GetHudPlayerParams() const { return HudPlayerParams; }

private:
	void BroadcastInitialValues() const;
	void BindCallbacksToDependencies();


	UPROPERTY()
	TObjectPtr<UGeoUserWidget> OverlayWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGeoUserWidget> OverlayWidgetClass;

	FHudPlayerParams HudPlayerParams;

	// Delegate
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAttributeModifiedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAttributeModifiedSignature OnMaxHealthChanged;
};
