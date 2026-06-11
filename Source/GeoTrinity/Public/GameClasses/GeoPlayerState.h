// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"

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

	/** Returns the rolling average damage-per-second over the last 10 seconds (replicated, for HUD display). */
	float GetDebugDPS() const { return DebugDPS; }
	/** Returns the rolling average healing-per-second over the last 10 seconds (replicated, for HUD display). */
	float GetDebugHPS() const { return DebugHPS; }
	/** Returns the highest rolling DPS reached this session. */
	float GetBestDPS() const { return BestDPS; }
	/** Returns the highest rolling HPS reached this session. */
	float GetBestHPS() const { return BestHPS; }
	/** Returns the cumulative damage dealt over the session. */
	float GetTotalDamageDealt() const { return TotalDamageDealt; }
	/** Returns the cumulative healing dealt over the session. */
	float GetTotalHealingDealt() const { return TotalHealingDealt; }
	/** Returns the cumulative damage received over the session. */
	float GetTotalDamageReceived() const { return TotalDamageReceived; }

	/**
	 * Batch-updates all replicated combat stat fields in one call. Called by UGeoCombatStatsSubsystem on the server.
	 *
	 * @param DPS      Rolling damage-per-second average.
	 * @param HPS      Rolling healing-per-second average.
	 * @param MaxDPS   Highest rolling DPS reached this session.
	 * @param MaxHPS   Highest rolling HPS reached this session.
	 * @param TotDmg   Cumulative damage dealt this session.
	 * @param TotHeal  Cumulative healing dealt this session.
	 * @param TotRecv  Cumulative damage received this session.
	 */
	void SetDebugCombatStats(float DPS, float HPS, float MaxDPS, float MaxHPS, float TotDmg, float TotHeal,
							 float TotRecv)
	{
		DebugDPS = DPS;
		DebugHPS = HPS;
		BestDPS = MaxDPS;
		BestHPS = MaxHPS;
		TotalDamageDealt = TotDmg;
		TotalHealingDealt = TotHeal;
		TotalDamageReceived = TotRecv;
	}

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UCharacterAttributeSet> CharacterAttributeSet;

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
	float TotalDamageDealt = 0.f;
	UPROPERTY(Replicated)
	float TotalHealingDealt = 0.f;
	UPROPERTY(Replicated)
	float TotalDamageReceived = 0.f;
};
