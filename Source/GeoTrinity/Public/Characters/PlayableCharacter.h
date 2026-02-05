#pragma once

#include "CoreMinimal.h"
#include "GeoCharacter.h"
#include "PlayerClassTypes.h"

#include "PlayableCharacter.generated.h"

UCLASS()
class GEOTRINITY_API APlayableCharacter : public AGeoCharacter
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaSeconds) override;

	// GAS - Input callbacks
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	EPlayerClass GetPlayerClass() const;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// GAS //
	virtual void InitGAS() override;
	// END GAS //
private:
	void UpdateAimRotation();

private:
	// Aim rotation cache to throttle RPCs
	float LastSentAimYaw = 0.f;
};
