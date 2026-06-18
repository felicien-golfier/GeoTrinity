// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "GeoCharacter.h"
#include "PlayerClassTypes.h"

#include "PlayableCharacter.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UMaterialInterface;
class UGameplayEffect;
class UWidgetComponent;
class UGeoChargeAbility;
class UGeoDeployableManagerComponent;

USTRUCT(BlueprintType)
struct FPlayerClassData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> AliveMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> DeathMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UAnimInstance> AnimClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DefaultAttributes;
};

/**
 * Human-controlled character. Bridges Enhanced Input with the GAS ability activation pipeline and
 * manages class switching (Square/Circle/Triangle) which swaps mesh, animations, and ability sets at runtime.
 */
UCLASS()
class GEOTRINITY_API APlayableCharacter : public AGeoCharacter
{
	GENERATED_BODY()
public:
	/** Creates widget components for the deploy and charge-beam gauges, and the deployable manager component. */
	APlayableCharacter(FObjectInitializer const& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Forwards an input-press event to the ASC for ability activation. */
	void AbilityInputTagPressed(FGameplayTag InputTag);

	/** Forwards an input-release event to the ASC. */
	void AbilityInputTagReleased(FGameplayTag InputTag);

	/** Forwards a held-input event to the ASC each frame the button is held. */
	void AbilityInputTagHeld(FGameplayTag InputTag);

	/** Returns the player's currently active class (Square, Circle, or Triangle). */
	EPlayerClass GetPlayerClass() const;

	/** Server. Revives a downed player: cancels active abilities, removes all gameplay effects, re-applies per-class default attributes, and restores the character. */
	void Revive();

	/** Returns true while the player is downed (health reached 0 and not yet revived). */
	bool IsDead() const { return bIsDead; }

	/**
	 * Switches the player to NewClass: clears current class abilities, applies new class data, and grants new
	 * abilities.
	 *
	 * @param NewClass  The target player class to switch to.
	 */
	void ChangeClass(EPlayerClass NewClass);

	/**
	 * Applies the mesh and animation class for NewClass without touching abilities.
	 * Called internally by ChangeClass and at initialization.
	 */
	void ApplyClassData(EPlayerClass NewClass);

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

	UFUNCTION()
	void OnHealthChanged(float NewValue);

	/** Server. Puts the player in the downed state: stops spawned elements and the character, notifies the GameState.
	 */
	void Death();

	/** Disables controls and collision and swaps to the death material. */
	void StopCharacter();

	/** Mirror of StopCharacter: restores controls, collision, and the alive material. */
	void RestartCharacter();

	UFUNCTION()
	void OnRep_IsDead(bool bOldValue);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Rotation",
			  meta = (ClampMin = "1.0", UIMin = "10.0"))
	float MaxRotationSpeed = 720.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UWidgetComponent> DeployChargeGaugeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UWidgetComponent> ChargeBeamGaugeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TMap<EPlayerClass, FPlayerClassData> ClassData;

private:
	void UpdateAimRotation(float DeltaSeconds) const;
	EPlayerClass PickStartingClass() const;
	/** Swaps the mesh material to the current class's death (bDead) or alive material. */
	void SetDeathMaterial(bool bDead);

	float PreviousYaw = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;
	FTimerHandle ChargeDeployHideTimerHandle;
	FTimerHandle ChargeBeamHideTimerHandle;
};
