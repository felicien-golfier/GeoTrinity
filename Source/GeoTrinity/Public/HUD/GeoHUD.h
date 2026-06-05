// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameplayTagContainer.h"

#include "GeoHUD.generated.h"

class UTexture2D;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeployCountChangedSignature, int32, CurrentCount, int32, MaxCount);


/** One ability-bar slot's static data: which ability it represents, its input, icon, and whether to show a deploy count. */
USTRUCT(BlueprintType)
struct FGeoAbilityBarEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag InputTag;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D const> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bIsDeployable = false;
};


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

	/** Returns PlayerController cast to AGeoPlayerController, or nullptr on type mismatch. */
	AGeoPlayerController* GetGeoPlayerController() const;
	/** Returns AbilitySystemComponent cast to UGeoAbilitySystemComponent, or nullptr on type mismatch. */
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponent() const;
	/** Returns AttributeSet cast to UGeoAttributeSetBase, or nullptr on type mismatch. */
	UGeoAttributeSetBase* GetGeoAttributeSet() const;
};


/**
 * HUD for GeoTrinity. Owns the player's OverlayWidget and an optional boss health bar.
 * Receives attribute change notifications from the ASC and forwards them as Blueprint-assignable delegates
 * so BP widgets can bind without needing a direct ASC reference.
 */
UCLASS()
class GEOTRINITY_API AGeoHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Creates and adds the OverlayWidget, then binds all attribute-change callbacks. Call once from AGeoPlayerState. */
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

	/** Returns the cached player parameters (controller, state, ASC, attribute set) set during InitOverlay. */
	FHudPlayerParams const& GetHudPlayerParams() const { return HudPlayerParams; }

	/** Shows the boss health bar for the given enemy. Call this when a boss fight starts. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void ShowBossHealthBar(AEnemyCharacter* Boss);

	/** Hides the boss health bar. Call this when the boss fight ends. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void HideBossHealthBar();

	/**
	 * Returns one entry per granted non-passive player ability, with icon/input/deployable flags resolved
	 * from the global UAbilityInfo asset. Used by the ability bar widget to build its slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityBar")
	TArray<FGeoAbilityBarEntry> GetAbilityBarEntries() const;

	/** Outputs the remaining cooldown and full cooldown duration (seconds) for the granted ability with AbilityTag. */
	UFUNCTION(BlueprintPure, Category = "AbilityBar")
	void GetAbilityCooldown(FGameplayTag AbilityTag, float& OutRemaining, float& OutDuration) const;

	/** Outputs the live and maximum deployable count for the deploy ability with AbilityTag, read from the avatar's manager. */
	UFUNCTION(BlueprintPure, Category = "AbilityBar")
	void GetDeployCountForAbility(FGameplayTag AbilityTag, int32& OutCurrent, int32& OutMax) const;

	/** Implemented in the HUD BP: forwards to the overlay's ability-bar widget to build slots from GetAbilityBarEntries. */
	UFUNCTION(BlueprintImplementableEvent, Category = "AbilityBar")
	void InitAbilityBar();

	/** Implemented in the HUD BP: rebuilds the ability bar after a class change re-grants abilities. */
	UFUNCTION(BlueprintImplementableEvent, Category = "AbilityBar")
	void RefreshAbilityBar();


protected:

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
