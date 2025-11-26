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

void APlayableCharacter::InitAbilityActorInfo()
{
	// DO NOT CALL SUPER !

	// Set the ASC for clients. Server does this in PossessedBy.
	AGeoPlayerState* GeoPlayerState = GetPlayerState<AGeoPlayerState>();
	checkf(IsValid(GeoPlayerState), TEXT("GeoPlayerState is not valid at replication !"));
	InitAbilityActorInfo(Cast<UGeoAbilitySystemComponent>(GeoPlayerState->GetAbilitySystemComponent()), GeoPlayerState,
		GeoPlayerState->GetGeoAttributeSetBase());
}

void APlayableCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
}

void APlayableCharacter::InitAbilityActorInfo(UGeoAbilitySystemComponent* GeoAbilitySystemComponent, AActor* OwnerActor,
	UCharacterAttributeSet* GeoAttributeSetBase)
{
	Super::InitAbilityActorInfo(GeoAbilitySystemComponent, OwnerActor, GeoAttributeSetBase);
	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GetController()))
	{
		// Hud only present locally
		if (AGeoHUD* Hud = Cast<AGeoHUD>(GeoPlayerController->GetHUD()))
		{
			Hud->InitOverlay(GeoPlayerController, GetPlayerState(), GeoAbilitySystemComponent, GeoAttributeSetBase);
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