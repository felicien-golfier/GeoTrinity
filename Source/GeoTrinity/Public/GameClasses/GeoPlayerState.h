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
	AGeoPlayerState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void ClientInitialize(AController* Controller) override;
	/** Creates the HUD overlay widget on the owning client. Called from ClientInitialize once the controller is valid. */
	void InitOverlay();

	/** Callback for APlayerState::PawnSetDelegate. Triggers InitGAS on the pawn when it is first assigned. */
	UFUNCTION()
	void OnPlayerPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	/** Implement IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** END Implement IAbilitySystemInterface */

	// IGenericTeamAgentInterface BEGIN
	virtual FGenericTeamId GetGenericTeamId() const override;
	// IGameplayTaskOwnerInterface END

	/** Returns the player's character attribute set (player-only attributes: ammo, multipliers, etc.). */
	UCharacterAttributeSet* GetCharacterAttributeSet() const { return CharacterAttributeSet; }
	/** Returns the GeoTrinity-specific ASC owned by this player state. */
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Returns the player's current playable class. */
	EPlayerClass GetPlayerClass() const { return PlayerClass; }
	/** Sets the player's current playable class. Does not grant or remove abilities — call GiveStartupAbilities separately. */
	void SetPlayerClass(EPlayerClass NewClass) { PlayerClass = NewClass; }

	/** Returns the exponentially smoothed damage-per-second (replicated, for HUD display). */
	float GetDebugDPS() const { return DebugDPS; }
	/** Returns the exponentially smoothed healing-per-second (replicated, for HUD display). */
	float GetDebugHPS() const { return DebugHPS; }
	/** Returns the highest smoothed DPS reached this combat. */
	float GetBestDPS() const { return BestDPS; }
	/** Returns the highest smoothed HPS reached this combat. */
	float GetBestHPS() const { return BestHPS; }
	/** Returns the average damage-per-second over the whole current combat (fight start until now). */
	float GetFightDPS() const { return FightDPS; }
	/** Returns the average healing-per-second over the whole current combat (fight start until now). */
	float GetFightHPS() const { return FightHPS; }
	/** Returns the cumulative damage dealt over the combat. */
	float GetTotalDamageDealt() const { return TotalDamageDealt; }
	/** Returns the cumulative healing dealt over the combat. */
	float GetTotalHealingDealt() const { return TotalHealingDealt; }
	/** Returns the cumulative damage received over the combat. */
	float GetTotalDamageReceived() const { return TotalDamageReceived; }

	/**
	 * Batch-updates all replicated combat stat fields in one call. Called by UGeoCombatStatsSubsystem on the server.
	 *
	 * @param DPS      Exponentially smoothed damage-per-second.
	 * @param HPS      Exponentially smoothed healing-per-second.
	 * @param MaxDPS   Highest smoothed DPS reached this combat.
	 * @param MaxHPS   Highest smoothed HPS reached this combat.
	 * @param AvgDPS   Average DPS over the whole current combat.
	 * @param AvgHPS   Average HPS over the whole current combat.
	 * @param TotDmg   Cumulative damage dealt this combat.
	 * @param TotHeal  Cumulative healing dealt this combat.
	 * @param TotRecv  Cumulative damage received this combat.
	 */
	void SetDebugCombatStats(float DPS, float HPS, float MaxDPS, float MaxHPS, float AvgDPS, float AvgHPS, float TotDmg,
							 float TotHeal, float TotRecv)
	{
		DebugDPS = DPS;
		DebugHPS = HPS;
		BestDPS = MaxDPS;
		BestHPS = MaxHPS;
		FightDPS = AvgDPS;
		FightHPS = AvgHPS;
		TotalDamageDealt = TotDmg;
		TotalHealingDealt = TotHeal;
		TotalDamageReceived = TotRecv;
	}

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCharacterAttributeSet> CharacterAttributeSet;

	/** Team the player belongs to. Owned here (not delegated to the pawn) so attitude queries resolve even when the
	 *  pawn link is momentarily absent (respawn, possession order on the server). */
	ETeam TeamId = ETeam::Player;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_PlayerClass, BlueprintReadOnly, Category = "Class")
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
	float DebugDPS = 0.f;
	UPROPERTY(Replicated)
	float DebugHPS = 0.f;
	UPROPERTY(Replicated)
	float BestDPS = 0.f;
	UPROPERTY(Replicated)
	float BestHPS = 0.f;
	UPROPERTY(Replicated)
	float FightDPS = 0.f;
	UPROPERTY(Replicated)
	float FightHPS = 0.f;
	UPROPERTY(Replicated)
	float TotalDamageDealt = 0.f;
	UPROPERTY(Replicated)
	float TotalHealingDealt = 0.f;
	UPROPERTY(Replicated)
	float TotalDamageReceived = 0.f;
};
