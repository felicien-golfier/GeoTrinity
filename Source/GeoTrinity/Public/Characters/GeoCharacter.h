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
	AGeoCharacter(FObjectInitializer const& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called every frame. */
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


	/** Entry point for reviving a downed player. Sets bIsDead = false and delegates to ReviveLogic(). */
	void Revive();

	/** Returns true while the player is downed (health reached 0 and not yet revived). */
	bool IsDead() const { return bIsDead; }

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

	/** Entry point for downing a player. Sets bIsDead = true and delegates to DeathLogic(). Called from
	 * OnHealthChanged. */
	void Death();
	/** Server. Puts the player in the downed state: stops spawned elements and the character, notifies the GameState.
	 */
	virtual void DeathLogic();

	/** Server. Revives a downed player: cancels active abilities, removes all gameplay effects, re-applies per-class
	 * default attributes, and restores the character. */
	virtual void ReviveLogic();

	UFUNCTION()
	void OnRep_IsDead(bool bOldValue);


	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;


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
	TObjectPtr<USceneComponent> WidgetAnchorComponent;

	// World-space health bar. Held as the engine base; the concrete UGeoCombattantWidgetComp (UI module) is set as the
	// default subobject class from GameDataSettings so gameplay never names it. Edit per-BP in the component tree.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UWidgetComponent> CharacterWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameFeel")
	TObjectPtr<UGeoGameFeelComponent> GameFeelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deployable")
	TObjectPtr<UGeoDeployableManagerComponent> DeployableManagerComponent;

#ifdef UE_EDITOR
private:
	ENetRole LocalRoleForDebugPurpose = ROLE_None;
#endif
};
