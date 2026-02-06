#include "AbilitySystem/Abilities/Common/GeoDashAbility.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGeoDashAbility::UGeoDashAbility()
{
	StartupInputTag = FGeoGameplayTags::Get().InputTag_Dash;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGeoDashAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!IsValid(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	FVector const DashDirection = GetDashDirection(ActorInfo, MovementComponent);
	float const DashSpeed = DashDistance / DashDuration;
	FVector const LaunchVelocity = DashDirection * DashSpeed;

	Character->LaunchCharacter(LaunchVelocity, true, true);

	ensureMsgf(!DashTimerHandle.IsValid(),
			   TEXT("Activating Dash with DashTimerHandle already valid ! The ability has not ended properly"));
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this,
								 [this, Handle, ActorInfo, ActivationInfo, DashDirection]()
								 {
									 ACharacter* DashCharacter = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
									 if (IsValid(DashCharacter))
									 {
										 DashCharacter->GetCharacterMovement()->StopMovementImmediately();
										 DashCharacter->GetCharacterMovement()->Velocity =
											 DashDirection * DashCharacter->GetCharacterMovement()->MaxWalkSpeed;
									 }
									 EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
								 });

	Character->GetWorldTimerManager().SetTimer(DashTimerHandle, TimerDelegate, DashDuration, false);
}

void UGeoDashAbility::EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo const ActivationInfo, bool const bReplicateEndAbility,
								 bool const bWasCancelled)
{
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (IsValid(Character))
	{
		Character->GetWorldTimerManager().ClearTimer(DashTimerHandle);
		UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
		if (IsValid(MovementComponent))
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector UGeoDashAbility::GetDashDirection(FGameplayAbilityActorInfo const* ActorInfo,
										  UCharacterMovementComponent const* MovementComponent) const
{
	FVector Direction = MovementComponent->GetLastUpdateVelocity();
	if (Direction.IsNearlyZero(0.1))
	{
		ACharacter const* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
		if (!IsValid(Character))
		{
			Direction = FVector::ForwardVector;
		}
		else
		{
			Direction = Character->GetActorForwardVector();
		}
	}

	Direction.Z = 0.f;
	Direction.Normalize();
	return Direction.IsNearlyZero() ? FVector::ForwardVector : Direction;
}
