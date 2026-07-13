// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameplayTagContainer.h"
#include "HUD/Interface/GeoHUDInterface.h"

#include "GeoHUD.generated.h"

class SWidget;
class UTexture2D;
class UInputAction;
class UGeoUserWidget;
class UGeoAttributeSetBase;
class AGeoPlayerController;
class UAttributeSet;
class UAbilitySystemComponent;
class UGeoAbilitySystemComponent;
class APlayerState;
class UGenericCombattantWidget;
class UGeoDamageNumberWidget;
class AEnemyCharacter;
class APlayableCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeModifiedSignature, float, NewValue);
/** Tagless "a deploy count changed" ping. Slots re-query GetDeployCountForAbility for their own tag on receipt. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeployCountChangedSignature);


/** One ability-bar slot's static data: which ability it represents, its input, icon, and whether to show a deploy
 * count. */
USTRUCT(BlueprintType)
struct FGeoAbilityBarEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag InputTag;

	/** Input action this slot is bound to; used to query the live mapped key for the slot's key label. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UInputAction const> InputAction = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D const> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bIsDeployable = false;
};


/** Active effects sharing the same icon on the local player, grouped for the status bar: stack count and the longest
 * remaining time (-1 when any instance is infinite). */
USTRUCT(BlueprintType)
struct FGeoActiveEffectIcon
{
	GENERATED_BODY()

	/** UTexture2D or UMaterialInterface, as configured on the applying effect data or status. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> Icon;

	UPROPERTY(BlueprintReadOnly)
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly)
	float TimeRemaining = 0.f;

	/** Longest total duration among stacked instances; -1 when TimeRemaining is infinite. Used to normalize the
	 * status bar's depletion sweep. */
	UPROPERTY(BlueprintReadOnly)
	float Duration = 0.f;
};


/** Snapshot of per-player GAS references cached by AGeoHUD after InitOverlay. Avoids repeated controller/pawn lookups
 * at HUD draw time. */
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
class GEOTRINITYUI_API AGeoHUD
	: public AHUD
	, public IGeoHUDInterface
{
	GENERATED_BODY()

public:
	/** Creates and adds the OverlayWidget, then binds all attribute-change callbacks. Call once from AGeoPlayerState.
	 */
	virtual void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC,
							 UAttributeSet* AS) override;

	/** Returns the cached player parameters (controller, state, ASC, attribute set) set during InitOverlay. */
	FHudPlayerParams const& GetHudPlayerParams() const { return HudPlayerParams; }

	/** Binds pawn-dependent HUD callbacks (deploy-count ping). Call once the local pawn exists, from OnPlayerPawnSet.
	 */
	virtual void BindToPawn(APlayableCharacter* PlayableCharacter) override;

	/** Shows the boss health bar for the given enemy. Call this when a boss fight starts. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	virtual void ShowBossHealthBar(AEnemyCharacter* Boss) override;

	/** Hides the boss health bar. Call this when the boss fight ends. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	virtual void HideBossHealthBar() override;

	/**
	 * Returns one entry per granted non-passive player ability, with icon/input/deployable flags resolved
	 * from the global UAbilityInfo asset. Used by the ability bar widget to build its slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityBar")
	TArray<FGeoAbilityBarEntry> GetAbilityBarEntries(APlayableCharacter* PlayableCharacter) const;

	/** Outputs the remaining cooldown and full cooldown duration (seconds) for the granted ability with AbilityTag. */
	UFUNCTION(BlueprintPure, Category = "AbilityBar")
	void GetAbilityCooldown(FGameplayTag AbilityTag, float& OutRemaining, float& OutDuration) const;

	/** Returns true while any instance of the granted ability with AbilityTag is active. */
	UFUNCTION(BlueprintPure, Category = "AbilityBar")
	bool IsAbilityActive(FGameplayTag AbilityTag) const;

	/** Returns true when the granted ability with AbilityTag passes its activation checks (tags, cost, cooldown).
	 * Lets a shared-input slot pick which of its abilities to display (e.g. sacrifice channel vs detonate). */
	UFUNCTION(BlueprintPure, Category = "AbilityBar")
	bool CanActivateAbility(FGameplayTag AbilityTag) const;

	/** Outputs the live and maximum deployable count for the deploy ability with AbilityTag, read from the avatar's
	 * manager. */
	UFUNCTION(BlueprintPure, Category = "AbilityBar")
	void GetDeployCountForAbility(FGameplayTag AbilityTag, int32& OutCurrent, int32& OutMax) const;

	/**
	 * (Re)builds the overlay's ability-bar slots from GetAbilityBarEntries. Called from BindToPawn once the pawn exists
	 * and from AGeoPlayerState::OnRep_PlayerClass after a class change re-grants abilities.
	 */
	virtual void BuildAbilityBar(APlayableCharacter* PlayableCharacter) override;

	/** Tagless ping fired when any deployable count changes; ability-bar slots re-query their own count on receipt (no
	 * polling). */
	UPROPERTY(BlueprintAssignable, Category = "AbilityBar")
	FOnDeployCountChangedSignature OnPlayerDeployCountChanged;

	/** Returns every active gameplay effect on the player ASC whose context carries an icon (set on
	 * FGameplayEffectData::Icon or the status's UStatusInfo icon), grouped by icon with stack count and the longest
	 * remaining time. Polled by UGeoStatusBarWidget. */
	UFUNCTION(BlueprintCallable, Category = "StatusBar")
	TArray<FGeoActiveEffectIcon> GetActiveEffectIcons() const;

	/** Binds Health and Shield attribute delegates on ASC so delta changes spawn floating numbers at OwnerActor. */
	void RegisterASCForDamageNumbers(UAbilitySystemComponent* ASC, AActor* OwnerActor);
	/** Finds or creates a pooled UGeoDamageNumberWidget and activates it at the projected screen position. */
	void SpawnDamageNumber(float Amount, bool bIsHeal, FVector WorldLocation);


protected:
#if !UE_BUILD_SHIPPING
	/** Rebuilds the debug combat-stats panel when the active player list changes. */
	virtual void DrawHUD() override;
	/** Removes the combat-stats panel from the viewport before calling Super. */
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
#endif


private:
#if !UE_BUILD_SHIPPING
	/** Creates, rebuilds, or removes the combat-stats panel so it mirrors the cvar and the current player list. */
	void UpdateCombatStatsPanel();
	/** Removes the Slate combat-stats panel from the viewport and resets panel state. */
	void RemoveCombatStatsPanel();

	/** Debug per-player DPS/HPS table, top-right of the viewport. Plain Slate: no widget Blueprint asset needed. */
	TSharedPtr<SWidget> CombatStatsPanel;
	int32 CombatStatsRowCount = 0;
#endif

	void BroadcastInitialValues() const;
	void BindCallbacksToDependencies();

	/** Bound to the avatar's deployable manager; fires the tagless OnPlayerDeployCountChanged ping (args ignored). */
	UFUNCTION()
	void HandleDeployCountChanged(int32 CurrentCount, int32 MaxCount);

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

	UPROPERTY(EditAnywhere, Category = "DamageNumbers")
	TSubclassOf<UGeoDamageNumberWidget> DamageNumberWidgetClass;

	UPROPERTY()
	TArray<TObjectPtr<UGeoDamageNumberWidget>> DamageNumberPool;

	UPROPERTY()
	TSet<TObjectPtr<UAbilitySystemComponent>> RegisteredDamageNumberASCs;

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
