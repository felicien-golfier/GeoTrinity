#pragma once

#include "CoreMinimal.h"
#include "GeoCharacter.h"

#include "PlayableCharacter.generated.h"

UCLASS()
class GEOTRINITY_API APlayableCharacter : public AGeoCharacter
{
	GENERATED_BODY()
public:
	APlayableCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaSeconds) override;

	// GAS - Input callbacks
	void AbilityInputTagPressed(FGameplayTag inputTag);
	void AbilityInputTagReleased(FGameplayTag inputTag);
	void AbilityInputTagHeld(FGameplayTag inputTag);

protected:
	void InitOverlay() const;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	// virtual void InitAbilityActorInfo() override;
	// virtual void InitAbilityActorInfo(UGeoAbilitySystemComponent* GeoAbilitySystemComponent, AActor* OwnerActor,
	// 	UCharacterAttributeSet* GeoAttributeSetBase) override;

private:
	void UpdateAimRotation();

private:
	// Aim rotation cache to throttle RPCs
	float LastSentAimYaw = 0.f;
};
