// Copyright 2024 GeoTrinity. All Rights Reserved.

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
class UGenericCombattantWidget;
class AEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeModifiedSignature, float, NewValue);


/** Groups the core player references needed by the HUD: controller, state, ASC, and attribute set. */
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

	/** Returns PlayerController cast to AGeoPlayerController, or nullptr if the cast fails. */
	AGeoPlayerController* GetGeoPlayerController() const;
	/** Returns AbilitySystemComponent cast to UGeoAbilitySystemComponent, or nullptr if the cast fails. */
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponent() const;
	/** Returns AttributeSet cast to UGeoAttributeSetBase, or nullptr if the cast fails. */
	UGeoAttributeSetBase* GetGeoAttributeSet() const;
};


/**
 * Main HUD actor for GeoTrinity. Owns the overlay widget (player HUD) and the boss health bar widget.
 * Initialized once from AGeoPlayerState::InitOverlay, then exposes attribute-change delegates that
 * the overlay widget BP connects to via BindCallbacksFromHUD.
 */
UCLASS()
class GEOTRINITY_API AGeoHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Creates the overlay widget and boss health bar widget, wires up ASC attribute callbacks, and fires BindCallbacksFromHUD. */
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

	/** Returns the cached player params (controller, state, ASC, attribute set) set during InitOverlay. */
	FHudPlayerParams const& GetHudPlayerParams() const { return HudPlayerParams; }

	/** Shows the boss health bar for the given enemy. Call this when a boss fight starts. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void ShowBossHealthBar(AEnemyCharacter* Boss);

	/** Hides the boss health bar. Call this when the boss fight ends. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void HideBossHealthBar();


protected:
	virtual void BeginPlay() override;

#if !UE_BUILD_SHIPPING
	virtual void DrawHUD() override;
#endif


private:
	void BroadcastInitialValues() const;
	void BindCallbacksToDependencies();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UGeoUserWidget> OverlayWidget;

private:
	UPROPERTY(EditAnywhere, Category = "Overlay")
	TSubclassOf<UGeoUserWidget> OverlayWidgetClass;

	/** Widget class for the boss health bar displayed at the top of the screen */
	UPROPERTY(EditAnywhere, Category = "Boss")
	TSubclassOf<UGenericCombattantWidget> BossHealthBarWidgetClass;

	UPROPERTY()
	TObjectPtr<UGenericCombattantWidget> BossHealthBarWidget;

	FHudPlayerParams HudPlayerParams;

	// Delegate
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAttributeModifiedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAttributeModifiedSignature OnMaxHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAttributeModifiedSignature OnAmmoChanged;
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAttributeModifiedSignature OnMaxAmmoChanged;
	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAttributeModifiedSignature OnShieldChanged;
};
