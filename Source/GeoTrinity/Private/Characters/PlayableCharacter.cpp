#include "Characters/PlayableCharacter.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoInputComponent.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GeoHUD.h"

class AGeoPlayerState;

void APlayableCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAimRotation(DeltaSeconds);
}
void APlayableCharacter::UpdateAimRotation(float DeltaSeconds)
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

void APlayableCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Set the ASC for clients. Server does this in PossessedBy.
	InitAbilityActorInfo();
}

void APlayableCharacter::InitAbilityActorInfo()
{
	Super::InitAbilityActorInfo();

	AGeoPlayerState* GeoPlayerState = GetPlayerState<AGeoPlayerState>();
	if (!GeoPlayerState)
	{
		return;
	}

	AbilitySystemComponent = Cast<UGeoAbilitySystemComponent>(GeoPlayerState->GetAbilitySystemComponent());
	AbilitySystemComponent->InitAbilityActorInfo(GeoPlayerState, this);
	AttributeSet = GeoPlayerState->GetGeoAttributeSetBase();

	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetController()))
	{
		// Hud only present locally
		if (AGeoHUD* Hud = Cast<AGeoHUD>(GeoPlayerController->GetHUD()))
		{
			Hud->InitOverlay(GeoPlayerController, GeoPlayerState, AbilitySystemComponent, AttributeSet);
		}
	}
}

void APlayableCharacter::AbilityInputTagPressed(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s pressed"), *inputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagPressed(inputTag);
}

void APlayableCharacter::AbilityInputTagReleased(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s released"), *inputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagReleased(inputTag);
}

void APlayableCharacter::AbilityInputTagHeld(FGameplayTag inputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s heeeeeld"), *inputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagHeld(inputTag);
}