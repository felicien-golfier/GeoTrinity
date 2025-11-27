#include "Characters/PlayableCharacter.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoInputComponent.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"

class AGeoPlayerState;

APlayableCharacter::APlayableCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InteractableComponent->bInitGasAtBeginPlay = false;
}

void APlayableCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAimRotation();
}

void APlayableCharacter::UpdateAimRotation()
{
	FVector2D Look;
	if (GeoInputComponent->GetLookVector(Look))
	{
		float DesiredYaw = FMath::Atan2(Look.Y, Look.X) * (180.f / PI);

		// Apply rotation locally
		FRotator R = GetActorRotation();
		R.Yaw = DesiredYaw;
		SetActorRotation(R);

		if (!HasAuthority())
		{
			constexpr float MinYawToUpdateServer = 0.5f;
			if (FMath::Abs(DesiredYaw - LastSentAimYaw) > MinYawToUpdateServer)
			{
				if (AGeoPlayerController* GC = Cast<AGeoPlayerController>(GetController()))
				{
					GC->ServerSetAimYaw(DesiredYaw);
					LastSentAimYaw = DesiredYaw;
				}
			}
		}
	}
}

void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	GeoInputComponent->BindInput(PlayerInputComponent);

	GeoInputComponent->BindAbilityActions(this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased,
		&ThisClass::AbilityInputTagHeld);
}

void APlayableCharacter::AbilityInputTagPressed(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s pressed"), *inputTag.ToString());
	if (!InteractableComponent->AbilitySystemComponent)
	{
		return;
	}
	InteractableComponent->AbilitySystemComponent->AbilityInputTagPressed(inputTag);
}

void APlayableCharacter::AbilityInputTagReleased(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s released"), *inputTag.ToString());
	if (!InteractableComponent->AbilitySystemComponent)
	{
		return;
	}
	InteractableComponent->AbilitySystemComponent->AbilityInputTagReleased(inputTag);
}

void APlayableCharacter::AbilityInputTagHeld(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s heeeeeld"), *inputTag.ToString());
	if (!InteractableComponent->AbilitySystemComponent)
	{
		return;
	}
	InteractableComponent->AbilitySystemComponent->AbilityInputTagHeld(inputTag);
}