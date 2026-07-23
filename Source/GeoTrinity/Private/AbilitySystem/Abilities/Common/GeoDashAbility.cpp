// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDashAbility.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

UGeoDashAbility::UGeoDashAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGeoDashAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	// Super commits the ability, validates cost/cooldown, and stores StoredPayload from
	// the activation target data (Origin, Yaw, ServerSpawnTime sent by the client).
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!IsValid(Character))
	{
		ensureMsgf(IsValid(Character), TEXT("UGeoDashAbility: invalid Character on activation"));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		ensureMsgf(IsValid(MovementComponent), TEXT("UGeoDashAbility: invalid MovementComponent on activation"));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	FVector const DashDirection = FRotator(0.f, StoredPayload.Yaw, 0.f).Vector();
	FVector const TargetLocation = Character->GetActorLocation() + DashDirection * DashDistance;

	// The task replicates the root motion source by ID and reconciles it through the CMC saved-move
	// system, owning the single authoritative timeout and the clamped finish velocity.
	UAbilityTask_ApplyRootMotionMoveToForce* DashTask =
		UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
			this, TEXT("Dash"), TargetLocation, DashDuration, false, MOVE_None, false, nullptr,
			ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, MovementComponent->MaxWalkSpeed);
	DashTask->OnTimedOut.AddDynamic(this, &UGeoDashAbility::OnDashFinished);
	DashTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGeoDashAbility::OnDashFinished);
	DashTask->ReadyForActivation();
}

void UGeoDashAbility::OnDashFinished()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}

float UGeoDashAbility::GetFireYaw(AActor const* Instigator, int const Seed) const
{
	ACharacter const* Character = Cast<ACharacter>(Instigator);
	if (IsValid(Character) && Character->GetVelocity().SizeSquared() >= SMALL_NUMBER)
	{
		return Character->GetVelocity().Rotation().Yaw;
	}

	return Super::GetFireYaw(Instigator, Seed);
}
