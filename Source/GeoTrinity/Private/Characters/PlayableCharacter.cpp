#include "Characters/PlayableCharacter.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Input/GeoInputComponent.h"

void APlayableCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateAimRotation(DeltaSeconds);
	}
	else if (DeltaSeconds > 0.f)
	{
		float const CurrentYaw = GetActorRotation().Yaw;
		float const InstantVelocity = FMath::FindDeltaAngleDegrees(PreviousYaw, CurrentYaw) / DeltaSeconds;
		constexpr float SmoothingSpeed = 1.f;
		YawVelocity = FMath::FInterpTo(YawVelocity, InstantVelocity, DeltaSeconds, SmoothingSpeed);
		PreviousYaw = CurrentYaw;
	}
}

void APlayableCharacter::UpdateAimRotation(float DeltaSeconds)
{
	FVector2D Look;
	if (!GeoInputComponent->GetLookVector(Look))
	{
		YawVelocity = 0.f;
		return;
	}

	float const DesiredYaw = FMath::Atan2(Look.Y, Look.X) * (180.f / PI);
	float const CurrentYaw = Controller->GetControlRotation().Yaw;
	float const DeltaAngle = FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw);
	float const MaxDelta = MaxRotationSpeed * DeltaSeconds;
	float const ClampedDelta = FMath::Clamp(DeltaAngle, -MaxDelta, MaxDelta);

	YawVelocity = (DeltaSeconds > 0.f) ? ClampedDelta / DeltaSeconds : 0.f;
	Controller->SetControlRotation(FRotator(0.f, CurrentYaw + ClampedDelta, 0.f));
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

EPlayerClass APlayableCharacter::GetPlayerClass() const
{
	if (AGeoPlayerState const* GeoPlayerState = GetPlayerState<AGeoPlayerState>())
	{
		return GeoPlayerState->GetPlayerClass();
	}
	return EPlayerClass::None;
}

void APlayableCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitGAS();
}
