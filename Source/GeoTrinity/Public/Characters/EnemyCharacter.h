// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/GeoCharacter.h"
#include "CoreMinimal.h"
#include "ScalableFloat.h"
#include "UObject/SoftObjectPtr.h"

#include "EnemyCharacter.generated.h"


class AGeoArena;
struct FGeoEnemySetup;
class UAnimMontage;
class UMaterialInterface;
class USkeletalMesh;
class UGeoLoopSoundComponent;
class UStateTree;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossDefeated);

/**
 * Enemy character controlled by a StateTree AI. Owns its ASC as a direct subobject (unlike
 * APlayableCharacter, where GAS lives on PlayerState). Boss death is handled in OnHealthChanged:
 * either resets health for looping attempts or broadcasts OnEnemyDefeated and self-destructs.
 */
UCLASS()
class GEOTRINITY_API AEnemyCharacter : public AGeoCharacter
{
	GENERATED_BODY()

public:
	/** Creates the ASC and attribute set as direct subobjects (enemies own their ASC, unlike players who use
	 * PlayerState). Sets AGeoEnemyAIController as the AI controller class. */
	AEnemyCharacter(FObjectInitializer const& ObjectInitializer);

	/** The tree the AI runs. Bosses share one base tree (the fight-start gate) and put their own spells in
	 * BehaviourStateTree. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacter|AI")
	TObjectPtr<UStateTree> StateTree;

	/** This enemy's own spell chain, run in StateTree's linked-asset state tagged AI.Boss.Behaviour. None = StateTree
	 * runs as authored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacter|AI")
	TObjectPtr<UStateTree> BehaviourStateTree;

	/** Played by AGeoArena::PlayIntro on its arena's first aggro, before the fight starts. None = no intro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacter|Boss")
	TObjectPtr<UAnimMontage> IntroMontage;

	/** The sound this enemy keeps running through its fight — a boss's hum. Its arena turns it on and off with the
	 * fight; montages bend it through their curves. Empty Loops = silent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCharacter|Boss")
	TObjectPtr<UGeoLoopSoundComponent> LoopSoundComponent;

	UPROPERTY(BlueprintAssignable, Category = "GeoCharacter|Boss")
	FOnBossDefeated OnEnemyDefeated;

	UPROPERTY(BlueprintReadOnly, Category = "GeoCharacter|Boss")
	TWeakObjectPtr<AGeoArena> Arena;

	/** True while this enemy's arena runs a fight, false before aggro and for an enemy with no arena. Reads the
	 * arena's replicated flag, so every machine gets the same answer. */
	UFUNCTION(BlueprintPure, Category = "GeoBoss")
	bool IsFighting() const;

	/** False: its controller never watches for players or damage, so no aggro and no fight start (a hidden caster). */
	virtual bool CanBeAggroed() const { return true; }

	/** FGeoEnemySetup::AggroRadius; negative = its controller's AggroRadius. */
	float AggroRadiusOverride = -1.f;

	/** Server. The one spawn of an enemy, arena bosses included: ArbitraryCharacterZ, unit scale, deferred.
	 * Arena (null for none) and Setup (null for none) are applied before FinishSpawning, so the controller's
	 * possession and the StateTree already see them. Null when the spawn failed. */
	static AEnemyCharacter* SpawnEnemy(UWorld& World, TSubclassOf<AEnemyCharacter> EnemyClass, FVector Location,
									   AActor* Owner, AGeoArena* Arena, FGeoEnemySetup const* Setup);

	/** Server. Applies the spawner's variant: team, dies or resets at zero, StateTree and BehaviourStateTree, extra
	 * abilities, health, actor tag. Between SpawnActorDeferred and FinishSpawning only, so the controller's possession
	 * and InitGAS see it. */
	void ApplySetup(FGeoEnemySetup const& Setup);

	/** Resets health to max and restarts the StateTree. Called on full-wipe to start a new attempt. */
	void ResetForNewAttempt();

	/** Body shown instead of the Blueprint's own while this enemy is on the player team (a friendly dummy). None =
	 * the body stays as it is. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCharacter|FriendlyLook")
	TObjectPtr<USkeletalMesh> FriendlyBodyMesh;

	/** Material put on the body's slot 0 while it is on the player team. None = the body keeps its material. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCharacter|FriendlyLook")
	TObjectPtr<UMaterialInterface> FriendlyBodyMaterial;

	/** True when the body wears the friendly look (the setup's team is the player's). */
	bool HasFriendlyLook() const { return bFriendlyLook; }

protected:
	virtual void BeginPlay() override;

	/** Registers bFriendlyLook (TeamId does not replicate). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Sets the combat level from the current GameState difficulty before calling the base GAS init. */
	virtual void InitGAS() override;

	/** Takes the enemy out of the fight — spawned elements, abilities, AI, movement, collision, damage — then lets the
	 * base play the death montage and destroy the actor once it has played out. */
	virtual void DeathLogic() override;

	/**
	 * Server-only gate on zero health: resets attributes to full when ResetToFullLifeWhenReachingZero is set,
	 * otherwise broadcasts OnEnemyDefeated and dies.
	 *
	 * @param NewValue  Current health value after the change.
	 */
	UFUNCTION(BlueprintNativeEvent)
	void OnHealthChanged(float NewValue);
	virtual void OnHealthChanged_Implementation(float NewValue);

private:
	/** Puts FriendlyBodyMesh and FriendlyBodyMaterial on the body. Same call on the server and on clients. */
	void ApplyFriendlyLook();

	UFUNCTION()
	void OnRep_FriendlyLook();

	/** Sets max health and health to MaxHealthOverride when it names one. */
	void ApplyMaxHealthOverride();

	/** Default attributes again, then the override. Always at full health. */
	void ResetAttributes();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "GeoCharacter|Boss")
	bool ResetToFullLifeWhenReachingZero = false;

	/** FGeoEnemySetup::MaxHealth; 0 = the DefaultAttributes' own. */
	FScalableFloat MaxHealthOverride;

	/** FGeoEnemySetup::StartHealthRatio; the first InitGAS only. */
	float StartHealthRatio = 1.f;

	UPROPERTY(ReplicatedUsing = OnRep_FriendlyLook)
	bool bFriendlyLook = false;
};
