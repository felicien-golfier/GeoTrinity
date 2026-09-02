// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystemInterface.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "CoreMinimal.h"
#include "GameClasses/GeoPlayerController.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"

#include "GeoCharacter.generated.h"


class UGeoDeployableManagerComponent;
enum class ETeam : uint8;
class UCharacterAttributeSet;
class UGeoGameplayAbility;
struct FGameplayTag;
class UGameplayEffect;
class UGeoAbilitySystemComponent;
class UGeoInputComponent;
class UDynamicMeshComponent;
class UGeoGameFeelComponent;
class UGeoCharacterMovementComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UAnimMontage;

/**
 * Abstract base character shared by APlayableCharacter and AEnemyCharacter.
 * Implements IAbilitySystemInterface and IGenericTeamAgentInterface, and exposes
 * helpers for input, movement, and ASC access that both subclasses need.
 * GAS initialization is deferred to InitGAS() which subclasses must override.
 */
UCLASS()
class GEOTRINITY_API AGeoCharacter
	: public ACharacter
	, public IAbilitySystemInterface
	, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	/**
	 * Substitutes UGeoCharacterMovementComponent via ObjectInitializer and creates default subobjects:
	 * GeoInputComponent, WidgetAnchorComponent, CharacterWidgetComponent (resolved from GameDataSettings;
	 * null on dedicated server), GameFeelComponent, and DeployableManagerComponent.
	 */
	AGeoCharacter(FObjectInitializer const& ObjectInitializer);
	/** Registers replicated character properties (bIsDead, bDiedFromFall). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Emits a visual-log entry at the character's location; optionally draws a debug sphere on the server
	 *  (Geo.ShowCharacterServerLocation CVar). */
	virtual void Tick(float DeltaSeconds) override;
	/** Expires all elements spawned by this character (deployables, and in future visual zones, etc). */
	void StopAllSpawnedElements();
	/** Calls StopAllSpawnedElements before delegating to Super. */
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;

	/** Returns the GeoInputComponent attached to this character. */
	UGeoInputComponent* GetGeoInputComponent() const { return GeoInputComponent; }
	/** Returns the movement component cast to UGeoCharacterMovementComponent. */
	UGeoCharacterMovementComponent* GetGeoMovementComponent() const
	{
		return Cast<UGeoCharacterMovementComponent>(GetMovementComponent());
	}

	//----------------------------------------------------------------------//
	// IAbilitySystemInterface BEGIN
	//----------------------------------------------------------------------//
	/** Returns the GAS component; required by IAbilitySystemInterface. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//----------------------------------------------------------------------//
	// IAbilitySystemInterface END
	//----------------------------------------------------------------------//

	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface BEGIN
	//----------------------------------------------------------------------//
	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(FGenericTeamId const& NewTeamId) override
	{
		TeamId = static_cast<ETeam>(NewTeamId.GetId());
	}

	/** Returns the team ID; required by IGenericTeamAgentInterface. */
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(static_cast<uint8>(TeamId)); };
	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface END
	//----------------------------------------------------------------------//

	/** Returns the controller cast to AGeoPlayerController, or nullptr if controlled by AI or a different type. */
	AGeoPlayerController* GetGeoPlayerController() const { return Cast<AGeoPlayerController>(GetController()); }

	/** Shows or hides this character's floating combatant widget (the bar above its head). Used to hide the boss's
	 *  floating bar while the dedicated on-screen boss bar is displayed. */
	void SetCombattantWidgetVisible(bool bVisible);

	/** Draws an arrow in the default debug color starting from the character's location. */
	void DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage) const;
	/** Draws an arrow in the given color starting from the character's location. */
	void DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage, FColor Color) const;


	/**
	 * Entry point for reviving a downed player. Plays GetReviveMontage() and only stands the character back up when it
	 * ends: bIsDead stays true and the character stays stopped for the whole montage, so nothing targets, damages or
	 * moves it while it gets up. Without a montage it revives on the spot.
	 */
	void Revive();

	/** Fires when this character revives, on the server (Revive) and on clients (OnRep_IsDead). Spawned elements that
	 * must not outlive a downed phase (e.g. shield burst projectiles) bind to this and end themselves. */
	FSimpleMulticastDelegate OnRevived;

	/** Returns true while the player is downed (health reached 0 and not yet revived). */
	bool IsDead() const { return bIsDead; }

	/** Returns true while the player is playing its revive montage — still down, but no longer a corpse. */
	bool IsReviving() const { return bReviving; }

	/**
	 * Entry point for downing a player. Sets bIsDead = true and delegates to DeathLogic(). Called from
	 * OnHealthChanged and from arena fall checks. No-op while Geo.PlayerInvincible is set.
	 *
	 * @param bFromFall  True when the character dropped into the void instead of running out of health. Replicated
	 *                   with bIsDead so DeathLogic picks the same montage on every machine.
	 */
	void Death(bool bFromFall = false);

	/** Sets the yaw (degrees) the character turns toward, at up to MaxRotationSpeed, in Tick. Callers (aim input,
	 * AI chase/move tasks) drive facing entirely through this — never through Controller::SetControlRotation or
	 * AIController::SetFocus, both of which snap rotation instantly. */
	void SetTargetYaw(float NewTargetYaw) { TargetYaw = NewTargetYaw; }

protected:
	virtual void BeginPlay() override;

	//----------------------------------------------------------------------//
	// GAS START
	//----------------------------------------------------------------------//

	/**
	 * Initializes the Gameplay Ability System for this character.
	 * Subclass implementations MUST call InitAbilityActorInfo with the correct owner and avatar actors.
	 */
	virtual void InitGAS();

	//----------------------------------------------------------------------//
	// GAS END
	//----------------------------------------------------------------------//

	/** Points the combatant health bar and the buff VFX at this character's ASC. Idempotent — call it from every point
	 *  the ASC or its attributes can first become available; the .cpp explains why no single one of them is enough. */
	void BindCosmeticsToASC();

	/** Server. Puts the player in the downed state: stops spawned elements and the character, notifies the GameState.
	 */
	virtual void DeathLogic();

	/** Server. Revives a downed player: cancels active abilities, removes all gameplay effects, re-applies per-class
	 * default attributes, and restores the character. */
	virtual void ReviveLogic();

	/** The whole revive sequence, shared by the server path (Revive) and the replicated one (OnRep_IsDead) — the
	 *  counterpart of DeathLogic() on the death side. */
	void HandleRevived();

	/** Server. Ends the getting-up state and revives for real. Revive montage timer callback. */
	void FinishRevive();

	UFUNCTION()
	void OnRep_IsDead(bool bOldValue);

	/** Plays the revive montage on every machine — the getting-up counterpart of SetDeathVisuals(true). */
	UFUNCTION()
	void OnRep_Reviving();

	/** Applies the visuals of a downed (bDead) or living character: plays or stops GetDeathMontage(). Runs on every
	 * machine — call it from the death/revive paths, which replicate through bIsDead. */
	virtual void SetDeathVisuals(bool bDead);

	/** Montage played while this character is down. Override where the montage varies with the character's state
	 * (a player's class swaps the skeleton the montage is bound to). */
	virtual UAnimMontage* GetDeathMontage() const { return DeathMontage; }

	/** Montage played while this character gets back up, and the length of the revive itself. None by default — a
	 *  character without one revives on the spot. */
	virtual UAnimMontage* GetReviveMontage() const { return nullptr; }


	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	/** True from the revive montage's first frame to its last, while bIsDead is still true — the character is on its
	 *  way back but counts as down until it is standing. */
	UPROPERTY(ReplicatedUsing = OnRep_Reviving)
	bool bReviving = false;

	/** How the current death happened, for GetDeathMontage(). Always re-assigned by Death(), so it never goes stale —
	 * and it must survive until ReviveLogic() stops the montage it selected. */
	UPROPERTY(Replicated)
	bool bDiedFromFall = false;

	/** Death montage of characters that keep one skeleton. Ignored where GetDeathMontage() is overridden. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCharacter|Animation")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	/** Max yaw turn rate in degrees/second, applied in Tick to close the gap toward TargetYaw. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCharacter|Rotation",
			  meta = (ClampMin = "1.0", UIMin = "10.0"))
	float MaxRotationSpeed = 720.f;

	/** Yaw (degrees) the character is currently turning toward. Set via SetTargetYaw(); initialized to the actor's
	 * starting yaw in BeginPlay so nothing snaps on possession. */
	float TargetYaw = 0.f;

	FTimerHandle ReviveTimer;


	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGeoAttributeSetBase> AttributeSetBase;

	UPROPERTY(Category = Team, EditAnywhere, BlueprintReadOnly)
	ETeam TeamId;

	// Non-rotating attachment point for all world widgets: their relative offsets would orbit the actor as the
	// capsule yaws if attached to the root (absolute rotation alone doesn't fix it — the offset is composed with the
	// parent rotation before the rotation override applies).
	UPROPERTY()
	TObjectPtr<USceneComponent> WidgetAnchorComponent;

	// World-space health bar. Held as the engine base; the concrete UGeoCombattantWidgetComp (UI module) is set as the
	// default subobject class from GameDataSettings so gameplay never names it. Edit per-BP in the component tree.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TObjectPtr<UWidgetComponent> CharacterWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoGameFeel")
	TObjectPtr<UGeoGameFeelComponent> GameFeelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoDeployable")
	TObjectPtr<UGeoDeployableManagerComponent> DeployableManagerComponent;

#if WITH_EDITOR
private:
	ENetRole LocalRoleForDebugPurpose = ROLE_None;
#endif
};
