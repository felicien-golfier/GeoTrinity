// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "GeoCharacter.h"
#include "PlayerClassTypes.h"

#include "PlayableCharacter.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UGameplayEffect;
class UWidgetComponent;
class UGeoChargeAbility;
class UGeoDeployableManagerComponent;
class UGeoChargeBeamGaugeWidget;

USTRUCT(BlueprintType)
struct FPlayerClassData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

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
	APlayableCharacter(FObjectInitializer const& ObjectInitializer);

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
	 * Applies the mesh and animation class for NewClass without touching abilities.
	 * Called internally by ChangeClass and at initialization.
	 */
	void ApplyClassData(EPlayerClass NewClass);

	/**
	 * Makes the deploy charge gauge widget visible and binds it to Ability's charge progress.
	 *
	 * @param Ability  The currently charging deploy ability that drives the gauge fill.
	 */
	void ShowDeployChargeGauge(UGeoGameplayAbility* Ability) const;

	/** Hides the deploy charge gauge widget. */
	void HideDeployChargeGauge() const;

	/**
	 * Shows or hides the charge-beam gauge widget.
	 *
	 * @param Ability            The charging ability that drives the gauge fill. Ignored when bVisible is false.
	 * @param bVisible           True to show and bind; false to hide.
	 * @param SweetSpotMinRatio  Sweet-spot window start (0–1). Only used when bVisible is true.
	 * @param SweetSpotMaxRatio  Sweet-spot window end (0–1). Only used when bVisible is true.
	 */
	void SetChargeBeamGaugeVisible(UGeoGameplayAbility* Ability, bool bVisible, float SweetSpotMinRatio = 0.f,
								   float SweetSpotMaxRatio = 0.f) const;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// GAS //
	virtual void InitGAS() override;
	// END GAS //

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Rotation",
			  meta = (ClampMin = "1.0", UIMin = "10.0"))
	float MaxRotationSpeed = 720.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UWidgetComponent> DeployChargeGaugeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UWidgetComponent> ChargeBeamGaugeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deployable")
	TObjectPtr<UGeoDeployableManagerComponent> DeployableManagerComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TMap<EPlayerClass, FPlayerClassData> ClassData;

private:
	void UpdateAimRotation(float DeltaSeconds) const;
	EPlayerClass PickStartingClass() const;

	float PreviousYaw = 0.f;
};
