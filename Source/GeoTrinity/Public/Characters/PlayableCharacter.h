#pragma once

#include "CoreMinimal.h"
#include "GeoCharacter.h"
#include "PlayerClassTypes.h"

#include "PlayableCharacter.generated.h"

class UWidgetComponent;
class UGeoDeployAbility;

UCLASS()
class GEOTRINITY_API APlayableCharacter : public AGeoCharacter
{
	GENERATED_BODY()
public:
	APlayableCharacter(FObjectInitializer const& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	// GAS - Input callbacks
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	EPlayerClass GetPlayerClass() const;

	void ShowDeployChargeGauge(UGeoDeployAbility* Ability);
	void HideDeployChargeGauge();

protected:
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

private:
	void UpdateAimRotation(float DeltaSeconds);

	float PreviousYaw = 0.f;
};
