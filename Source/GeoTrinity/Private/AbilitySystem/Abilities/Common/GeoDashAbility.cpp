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

	// Direction comes from StoredPayload.Yaw — identical on both client and server because
	// the client bakes its local yaw into the activation target data before sending.
	FVector const DashDirection = FRotator(0.f, StoredPayload.Yaw, 0.f).Vector();
	float const DashSpeed = DashDistance / DashDuration;

	Character->LaunchCharacter(DashDirection * DashSpeed, true, true);

	ensureMsgf(!DashTimerHandle.IsValid(),
			   TEXT("Activating Dash with DashTimerHandle already valid — ability has not ended properly"));
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
