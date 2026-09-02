// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"
#include "Tool/Team.h"

class APlayableCharacter;

#include "GeoPlayerState.generated.h"

class UCharacterAttributeSet;
class UGeoAbilitySystemComponent;

/**
 * Per-player combat figures pushed from UGeoCombatStatsSubsystem for HUD display. Replicated as one property
 * because the subsystem always writes the whole set together.
 */
USTRUCT()
struct FGeoCombatDisplayStats
{
	GENERATED_BODY()

	/** Exponentially smoothed damage-per-second. */
	UPROPERTY()
	float DebugDPS = 0.f;
	/** Exponentially smoothed healing-per-second. */
	UPROPERTY()
	float DebugHPS = 0.f;
	/** Biggest damage total dealt within one burst window this combat. */
	UPROPERTY()
	float MaxBurstDamage = 0.f;
	/** Biggest healing total dealt within one burst window this combat. */
	UPROPERTY()
	float MaxBurstHealing = 0.f;
	/** Average damage-per-second over the whole current combat. */
	UPROPERTY()
	float FightDPS = 0.f;
	/** Average healing-per-second over the whole current combat. */
	UPROPERTY()
	float FightHPS = 0.f;
	/** Cumulative damage dealt this combat. */
	UPROPERTY()
	float TotalDamageDealt = 0.f;
	/** Cumulative healing dealt this combat. */
	UPROPERTY()
	float TotalHealingDealt = 0.f;
	/** Cumulative damage received this combat. */
	UPROPERTY()
	float TotalDamageReceived = 0.f;
};

/**
 * Player state for GeoTrinity. Owns the ASC and UCharacterAttributeSet for playable characters
 * (ASC lives here so it survives pawn respawns). Also tracks the player's current class,
 * replicated combat debug stats (DPS, HPS, damage received), and triggers HUD overlay initialization.
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerState
	: public APlayerState
	, public IAbilitySystemInterface
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	/** Creates the ASC and attribute set that this player state owns and keeps alive across pawn respawns. */
	AGeoPlayerState();
	/** Registers OnMatchStateChanged delegate binding on the server for stat reset coordination. */
	virtual void BeginPlay() override;
	/** Registers PlayerClass and the combat stat block for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Binds PawnSetDelegate and calls InitOverlay once the owning controller is available on this machine. */
	virtual void ClientInitialize(AController* Controller) override;
	/** Creates the HUD overlay widget on the owning client. Called from ClientInitialize once the controller is valid. */
	void InitOverlay();

	/** Callback for APlayerState::PawnSetDelegate. Triggers InitGAS on the pawn when it is first assigned. */
	UFUNCTION()
	void OnPlayerPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	/** Returns the ASC owned by this player state; required by IAbilitySystemInterface. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Returns this player's team identifier for attitude queries; required by IGenericTeamAgentInterface. */
	virtual FGenericTeamId GetGenericTeamId() const override;

	/** Returns the player's character attribute set (player-only attributes: ammo, multipliers, etc.). */
	UCharacterAttributeSet* GetCharacterAttributeSet() const { return CharacterAttributeSet; }
	/** Returns the GeoTrinity-specific ASC owned by this player state. */
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Returns the player's current playable class. */
	EPlayerClass GetPlayerClass() const { return PlayerClass; }
	/** Sets the player's current playable class. Does not grant or remove abilities — call GiveStartupAbilities separately. */
	void SetPlayerClass(EPlayerClass NewClass) { PlayerClass = NewClass; }

	/** Returns the current exponentially smoothed damage-per-second rate; decays between events so it reads as a live rate, not a peak. */
	float GetDebugDPS() const { return CombatStats.DebugDPS; }
	/** Returns the current exponentially smoothed healing-per-second rate; decays between events so it reads as a live rate, not a peak. */
	float GetDebugHPS() const { return CombatStats.DebugHPS; }
	/** Returns the biggest damage total accumulated inside a single burst window this combat (largest single-spell landing, not a rate). */
	float GetMaxBurstDamage() const { return CombatStats.MaxBurstDamage; }
	/** Returns the biggest healing total accumulated inside a single burst window this combat (largest single-spell landing, not a rate). */
	float GetMaxBurstHealing() const { return CombatStats.MaxBurstHealing; }
	/** Returns the whole-combat average damage-per-second (cumulative damage divided by elapsed fight time). */
	float GetFightDPS() const { return CombatStats.FightDPS; }
	/** Returns the whole-combat average healing-per-second (cumulative healing divided by elapsed fight time). */
	float GetFightHPS() const { return CombatStats.FightHPS; }
	/** Returns the cumulative damage dealt by this player since the current combat began. */
	float GetTotalDamageDealt() const { return CombatStats.TotalDamageDealt; }
	/** Returns the cumulative healing dealt by this player since the current combat began. */
	float GetTotalHealingDealt() const { return CombatStats.TotalHealingDealt; }
	/** Returns the cumulative damage received by this player since the current combat began. */
	float GetTotalDamageReceived() const { return CombatStats.TotalDamageReceived; }

	/** Replaces every displayed combat stat in one write. Called by UGeoCombatStatsSubsystem on the server. */
	void SetDebugCombatStats(FGeoCombatDisplayStats const& NewStats) { CombatStats = NewStats; }

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCharacterAttributeSet> CharacterAttributeSet;

	/** Team the player belongs to. Owned here (not delegated to the pawn) so attitude queries resolve even when the
	 *  pawn link is momentarily absent (respawn, possession order on the server). */
	ETeam TeamId = ETeam::Player;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_PlayerClass, BlueprintReadOnly, Category = "GeoClass")
	EPlayerClass PlayerClass = EPlayerClass::None;

	UFUNCTION()
	void OnRep_PlayerClass();

	/** Applies the current PlayerClass visuals (mesh, anim, material) to the owned pawn, if it exists. */
	void ApplyClassDataToPawn();

public:
	/**
	 * Rebuilds the local HUD ability bar from the owned pawn's current ability set. No-op unless the owning controller
	 * is local. Call after abilities are (re)granted: from OnRep_PlayerClass on clients, and from
	 * APlayableCharacter::ChangeClass on the server/listen-host (which has no OnRep_PlayerClass).
	 */
	void RebuildAbilityBar();

protected:

	UPROPERTY(Replicated)
	FGeoCombatDisplayStats CombatStats;
};
