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
class UGeoDeployAbility;
class UGeoDeployableManagerComponent;

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
	void ChangeClass(EPlayerClass NewClass);
	void ApplyClassData(EPlayerClass NewClass);

	void ShowDeployChargeGauge(UGeoDeployAbility* Ability) const;
	void HideDeployChargeGauge() const;

	UCurveFloat* GetGaugeChargingSpeedCurve() const { return GaugeChargingSpeedCurve; }

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deployable")
	TObjectPtr<UGeoDeployableManagerComponent> DeployableManagerComponent;

	/** Curve to remap the raw charge ratio (0-1) and influence its charge speed.*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable")
	TObjectPtr<UCurveFloat> GaugeChargingSpeedCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TMap<EPlayerClass, FPlayerClassData> ClassData;

private:
	void UpdateAimRotation(float DeltaSeconds) const;
	EPlayerClass PickStartingClass() const;

	float PreviousYaw = 0.f;
};
