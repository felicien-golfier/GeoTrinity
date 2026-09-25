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
 * PrioritizeCategories also lists the subclasses' subcategories: they inherit this order, and one left out would sink
 * to the bottom of the GeoCharacter group.
 */
UCLASS(PrioritizeCategories = ("GeoCharacter|AI", "GeoCharacter|Boss", "GeoCharacter|Aim", "GeoCharacter|Movement",
							   "GeoCharacter|Death", "GeoCharacter|Team", "GeoCharacter|Components"))
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
	/** Registers replicated character properties (bIsDead, bInvulnerable). */
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
	/** Stores the team as ETeam so team-based attitude queries and collision filtering use the correct identity. */
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


	/** Entry point for reviving a downed player: clears bIsDead and runs HandleRevived(). No-op while alive. */
	void Revive();

	/** Fires when this character revives, on the server (Revive) and on clients (OnRep_IsDead). Spawned elements that
	 * must not outlive a downed phase (e.g. shield burst projectiles) bind to this and end themselves. */
	FSimpleMulticastDelegate OnRevived;

	/** Returns true while the character is down (health reached 0 and not yet revived, or dying for an enemy). */
	bool IsDead() const { return bIsDead; }

	/** Server. Server time of the last Death(). */
	float GetDeathServerTime() const { return DeathServerTime; }

	/**
	 * Entry point for a death. Sets bIsDead = true and delegates to DeathLogic(). Called from OnHealthChanged and from
	 * arena fall checks. Invulnerability deliberately does not guard this: it stops damage, not the void, and a
	 * character left over a destroyed tile still falls. A player's is a no-op while Geo.PlayerInvincible is set.
	 */
	void Death();

	/**
	 * Server. Makes this character untouchable: nothing can be applied to it — every targeting and hit path in the
	 * project gates on CanBeDamaged — its collision is off, and the effects others put on it are dropped, so nothing
	 * keeps ticking through the invulnerability. Its own effects (passives, buffs it applied to itself) are left
	 * standing. Replicated, so every half lands on every machine.
	 */
	void SetInvulnerable(bool bNewInvulnerable);

	/** True while nothing can damage, touch, or take health off this character. */
	bool IsInvulnerable() const { return bInvulnerable; }

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

	/** Runs on every machine (Death, OnRep_IsDead). Plays the death visuals; on the server the character is destroyed
	 * once its death montage has played out, at once without one. */
	virtual void DeathLogic();

	/** Server. Revives a downed player: cancels active abilities, removes all gameplay effects, re-applies per-class
	 * default attributes, and restores the character. */
	virtual void ReviveLogic();

	/** The whole revive sequence, shared by the server path (Revive) and the replicated one (OnRep_IsDead) — the
	 *  counterpart of DeathLogic() on the death side. */
	void HandleRevived();

	UFUNCTION()
	void OnRep_Invulnerable();

	/** Puts collision and the damageable state in sync with bInvulnerable. Called from the two places bInvulnerable
	 * changes — SetInvulnerable on the server, OnRep_Invulnerable on the clients — so both halves land everywhere. */
	void ApplyInvulnerability();

	UFUNCTION()
	void OnRep_IsDead(bool bOldValue);

	/** Plays GetDeathMontage() (bDead), or stops it. The montage never blends out on its own, so its last pose holds for
	 * the whole downed state. Runs on every machine — call it from the death/revive paths, which replicate through
	 * bIsDead. */
	void SetDeathVisuals(bool bDead);

	/** Montage played when this character dies. Override where it varies with the character's state (a player's class
	 * swaps the skeleton the montage is bound to). */
	virtual UAnimMontage* GetDeathMontage() const { return DeathMontage; }


	// Movement

	/** Max yaw turn rate in degrees/second, applied in Tick to close the gap toward TargetYaw. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCharacter|Movement",
			  meta = (ClampMin = "1.0", UIMin = "10.0"))
	float MaxRotationSpeed = 720.f;

	/** Yaw (degrees) the character is currently turning toward. Set via SetTargetYaw(); initialized to the actor's
	 * starting yaw in BeginPlay so nothing snaps on possession. */
	float TargetYaw = 0.f;

	// Death and invulnerability

	/** Death montage of characters that keep one skeleton (enemies). None = an enemy vanishes at once. Ignored where
	 * GetDeathMontage() is overridden. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCharacter|Death")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	float DeathServerTime = 0.f;

	/** True while this character can neither be hit nor affected. Driven by SetInvulnerable on the server. */
	UPROPERTY(ReplicatedUsing = OnRep_Invulnerable)
	bool bInvulnerable = false;

	// Team

	UPROPERTY(Category = "GeoCharacter|Team", EditAnywhere, BlueprintReadOnly)
	ETeam TeamId;

	// Components

	UPROPERTY(Category = "GeoCharacter|Components", EditAnywhere, BlueprintReadOnly,
			  meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(Category = "GeoCharacter|Components", EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGeoAttributeSetBase> AttributeSetBase;

	// Non-rotating attachment point for all world widgets: their relative offsets would orbit the actor as the
	// capsule yaws if attached to the root (absolute rotation alone doesn't fix it — the offset is composed with the
	// parent rotation before the rotation override applies).
	UPROPERTY()
	TObjectPtr<USceneComponent> WidgetAnchorComponent;

	// World-space health bar. Held as the engine base; the concrete UGeoCombattantWidgetComp (UI module) is set as the
	// default subobject class from GameDataSettings so gameplay never names it. Edit per-BP in the component tree.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCharacter|Components")
	TObjectPtr<UWidgetComponent> CharacterWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCharacter|Components")
	TObjectPtr<UGeoGameFeelComponent> GameFeelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCharacter|Components")
	TObjectPtr<UGeoDeployableManagerComponent> DeployableManagerComponent;

#if WITH_EDITOR
private:
	ENetRole LocalRoleForDebugPurpose = ROLE_None;
#endif
};
