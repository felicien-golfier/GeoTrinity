// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/Component/GeoDeploySatelliteComponent.h"
#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "GeoCharacter.h"
#include "PlayerClassTypes.h"

#include "PlayableCharacter.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UAnimMontage;
class UMaterialInterface;
class UGameplayEffect;
class UWidgetComponent;
class UGeoChargeAbility;
class UGeoDeployableManagerComponent;
class UGeoDeploySatelliteComponent;
class UPlayerClassDataAsset;
struct FPlayerClassData;

/**
 * Human-controlled character. Bridges Enhanced Input with the GAS ability activation pipeline and
 * manages class switching (Square/Circle/Triangle) which swaps mesh, animations, and ability sets at runtime.
 */
UCLASS()
class GEOTRINITY_API APlayableCharacter : public AGeoCharacter
{
	GENERATED_BODY()
public:
	/** Creates widget components for the deploy and charge-beam gauges and the aim cursor, the deployable manager
	 * component, and the deploy satellite ring. */
	APlayableCharacter(FObjectInitializer const& ObjectInitializer);

	/** Drives aim rotation toward the cursor and shows the aim cursor while a gamepad owns the aim. */
	virtual void Tick(float DeltaSeconds) override;

	/** Forwards an input-press event to the ASC for ability activation. */
	void AbilityInputTagPressed(FGameplayTag InputTag);

	/** Forwards an input-release event to the ASC. */
	void AbilityInputTagReleased(FGameplayTag InputTag);

	/** Forwards a held-input event to the ASC each frame the button is held. */
	void AbilityInputTagHeld(FGameplayTag InputTag);

	/** Returns the player's currently active class (Square, Circle, or Triangle). */
	EPlayerClass GetPlayerClass() const;

	/**
	 * Switches the player to NewClass: clears current class abilities, applies new class data, and grants new
	 * abilities.
	 *
	 * @param NewClass  The target player class to switch to.
	 */
	void ChangeClass(EPlayerClass NewClass);

	/**
	 * Applies the mesh, animation class, and material for NewClass — pure visuals, no attributes or abilities.
	 * Called by ChangeClass and by AGeoPlayerState::ApplyClassDataToPawn (runs on every client for every pawn).
	 */
	void ApplyClassData(EPlayerClass NewClass);

	/**
	 * Server-only. Brings the character to the alive state: re-applies the current class's DefaultAttributes
	 * (full HP/attributes) and starts passive abilities. Called at the end of ChangeClass and ReviveLogic.
	 */
	void GiveLife();

	/**
	 * Makes the deploy charge gauge widget visible and binds it to Ability's charge progress.
	 *
	 * @param Ability   The currently charging deploy ability that drives the gauge fill.
	 * @param bVisible  True to show and bind immediately; false to hide after a short delay.
	 */
	void SetDeployChargeGaugeVisibility(UGeoGameplayAbility* Ability, bool bVisible);

	/**
	 * Shows or hides the charge-beam gauge widget.
	 *
	 * @param Ability            The charging ability that drives the gauge fill. Ignored when bVisible is false.
	 * @param bVisible           True to show and bind; false to hide.
	 * @param SweetSpotMinRatio  Sweet-spot window start (0–1). Only used when bVisible is true.
	 * @param SweetSpotMaxRatio  Sweet-spot window end (0–1). Only used when bVisible is true.
	 */
	void SetChargeBeamGaugeVisible(UGeoGameplayAbility* Ability, bool bVisible, float SweetSpotMinRatio = 0.f,
								   float SweetSpotMaxRatio = 0.f);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// GAS //
	virtual void InitGAS() override;
	// END GAS //

	virtual void DeathLogic() override;
	virtual void ReviveLogic() override;

	UFUNCTION()
	void OnHealthChanged(float NewValue);

	/** Disables controls and collision and applies the death visuals. */
	void StopCharacter();

	/** Mirror of StopCharacter: restores controls, collision, and the alive visuals. */
	void RestartCharacter();

	/** Cancels every active ability and (server-only, replicated down) purges all active gameplay effects.
	 * Shared reset used by death, revive, and class change. */
	void ResetAbilitiesAndEffects();

	/** Adds the current class's death (bDead) or alive material on top of the base's montage handling. */
	virtual void SetDeathVisuals(bool bDead) override;

	/** Returns the current class's fall or death montage — each class brings its own skeleton, so the montages follow
	 * it. */
	virtual UAnimMontage* GetDeathMontage() const override;

	/** Returns the current class's revive montage, for the same reason GetDeathMontage() is overridden. */
	virtual UAnimMontage* GetReviveMontage() const override;

	/** Returns Class's authored data, or null with an ensure — a class the map has no entry for is a configuration bug.
	 *  The single answer to "what is this character's class data?", so every caller fails the same way. */
	FPlayerClassData const* GetClassData(EPlayerClass Class) const;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TObjectPtr<UWidgetComponent> DeployChargeGaugeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TObjectPtr<UWidgetComponent> ChargeBeamGaugeComponent;

	// This player's own crosshair, shown only while they aim with a gamepad — there is one mouse cursor, so every other
	// couch-coop player would have none. Attached to the rotating root, not WidgetAnchorComponent: this is the one
	// world widget whose offset is meant to orbit the actor, keeping it in front of the character as it turns toward
	// the aim.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TObjectPtr<UWidgetComponent> AimCursorComponent;

	// Sits on WidgetAnchorComponent, not the root: the satellites orbit the player themselves, so the ring must not
	// also be swung around by the character turning toward its aim.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoDeployable")
	TObjectPtr<UGeoDeploySatelliteComponent> DeploySatelliteComponent;

	/** Distance in world units from the character to its aim cursor. Applied in BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoHUD", meta = (ClampMin = "0.0"))
	float AimCursorDistance = 800.f;

private:
	void UpdateAimRotation(float DeltaSeconds);
	EPlayerClass PickStartingClass() const;

	/**
	 * Shows or hides one world-space charge gauge. Showing binds Ability and cancels any pending hide; hiding pushes a
	 * last fill update, detaches the ability and collapses the widget after GaugeHideDelay.
	 *
	 * @param Component   The gauge's widget component; its user widget must implement IGeoChargeGaugeWidgetInterface.
	 * @param HideHandle  Timer handle owning this gauge's delayed hide.
	 */
	void SetChargeGaugeVisible(UWidgetComponent* Component, FTimerHandle& HideHandle, UGeoGameplayAbility* Ability,
							   bool bVisible);

	/** Sets Material on mesh slot 0 and recreates the shield-burst gauge MID that the raw material set discards. */
	void SetBodyMaterial(UMaterialInterface* Material);

	float PreviousYaw = 0.f;
	FTimerHandle ChargeDeployHideTimerHandle;
	FTimerHandle ChargeBeamHideTimerHandle;
};
