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
class UGeoCombattantWidgetComp;

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
	virtual void Tick(float DeltaSeconds) override;
	/** Expires all elements spawned by this character (deployables, and in future visual zones, etc). */
	void StopAllSpawnedElements();
	/** Calls StopAllSpawnedElements before delegating to Super. */
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
	;
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

	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(static_cast<uint8>(TeamId)); };
	//----------------------------------------------------------------------//
	// IGenericTeamAgentInterface END
	//----------------------------------------------------------------------//

	/** Returns the controller cast to AGeoPlayerController, or nullptr if controlled by AI or a different type. */
	AGeoPlayerController* GetGeoPlayerController() const { return Cast<AGeoPlayerController>(GetController()); }

	/** Draws an arrow in the default debug color starting from the character's location. */
	void DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage) const;
	/** Draws an arrow in the given color starting from the character's location. */
	void DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage, FColor Color) const;

protected:
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

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGeoInputComponent> GeoInputComponent;

	UPROPERTY(Category = Geo, EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGeoAttributeSetBase> AttributeSetBase;

	UPROPERTY(Category = Team, EditAnywhere, BlueprintReadOnly)
	ETeam TeamId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UGeoCombattantWidgetComp> CharacterWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameFeel")
	TObjectPtr<UGeoGameFeelComponent> GameFeelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deployable")
	TObjectPtr<UGeoDeployableManagerComponent> DeployableManagerComponent;

#ifdef UE_EDITOR
public:
	virtual void BeginPlay() override;

private:
	ENetRole LocalRoleForDebugPurpose = ROLE_None;
#endif
};
