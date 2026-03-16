// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDashAbility.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Tool/UGameplayLibrary.h"

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

	// Direction comes from character velocity, if no, use Yaw.
	FVector DashDirection;
	if (MovementComponent->Velocity.SizeSquared() < SMALL_NUMBER)
	{
		DashDirection = FRotator(0.f, StoredPayload.Yaw, 0.f).Vector();
	}
	else
	{
		DashDirection = MovementComponent->Velocity.GetSafeNormal();
	}

	FVector const StartLocation = FVector(StoredPayload.Origin, ArbitraryCharacterZ);
	FVector const TargetLocation = StartLocation + DashDirection * DashDistance;

	// FRootMotionSource_MoveToForce is saved in FSavedMove_Character, so both client and
	// server apply identical movement when the server replays saved moves — no CMC corrections.
	TSharedPtr<FRootMotionSource_MoveToForce> DashRootMotion = MakeShared<FRootMotionSource_MoveToForce>();
	DashRootMotion->InstanceName = TEXT("Dash");
	DashRootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	DashRootMotion->StartLocation = StartLocation;
	DashRootMotion->TargetLocation = TargetLocation;
	DashRootMotion->Duration = DashDuration;
	DashRootMotion->bRestrictSpeedToExpected = false;
	DashRootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
	DashRootMotion->FinishVelocityParams.ClampVelocity = MovementComponent->MaxWalkSpeed;

	ensureMsgf(DashRootMotionSourceID == 0,
			   TEXT("Activating Dash with DashRootMotionSourceID already set — ability has not ended properly"));
	DashRootMotionSourceID = MovementComponent->ApplyRootMotionSource(DashRootMotion);

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this,
								 [this, Handle, ActorInfo, ActivationInfo]()
								 {
									 EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
								 });

	Character->GetWorldTimerManager().SetTimer(DashEndTimerHandle, TimerDelegate, DashDuration, false);
}

void UGeoDashAbility::EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo const ActivationInfo, bool const bReplicateEndAbility,
								 bool const bWasCancelled)
{
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (IsValid(Character))
	{
		Character->GetWorldTimerManager().ClearTimer(DashEndTimerHandle);
		UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
		if (IsValid(MovementComponent))
		{
			MovementComponent->RemoveRootMotionSourceByID(DashRootMotionSourceID);
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
	}
	DashRootMotionSourceID = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
