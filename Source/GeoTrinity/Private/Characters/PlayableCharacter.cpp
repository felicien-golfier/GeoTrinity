#include "Characters/PlayableCharacter.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Input/GeoInputComponent.h"

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

void APlayableCharacter::InitGAS()
{
	AGeoPlayerState* GeoPlayerState = GetPlayerState<AGeoPlayerState>();
	if (!GeoPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("No player state in %s"), *GetName());
		return;
	}

	AbilitySystemComponent = Cast<UGeoAbilitySystemComponent>(GeoPlayerState->GetAbilitySystemComponent());
	AbilitySystemComponent->InitAbilityActorInfo(GeoPlayerState, this);
	AttributeSetBase = GeoPlayerState->GetCharacterAttributeSet();

	Super::InitGAS();
}

void APlayableCharacter::AbilityInputTagPressed(FGameplayTag InputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s pressed"), *InputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagPressed(InputTag);
}

void APlayableCharacter::AbilityInputTagReleased(FGameplayTag InputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s released"), *InputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagReleased(InputTag);
}

void APlayableCharacter::AbilityInputTagHeld(FGameplayTag InputTag)
{
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("Ability tag %s heeeeeld"), *InputTag.ToString());
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->AbilityInputTagHeld(InputTag);
}

void APlayableCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Set the ASC on the Server. Clients do this in OnRep_PlayerState()
	InitGAS();
}

void APlayableCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitGAS();
}