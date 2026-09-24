// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDashAbility.h"

#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "GameFramework/Character.h"

void UGeoDashAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	UGeoCharacterMovementComponent* MovementComponent =
		Cast<UGeoCharacterMovementComponent>(ActorInfo->MovementComponent.Get());
	if (!ensureMsgf(IsValid(MovementComponent), TEXT("%hs: avatar has no UGeoCharacterMovementComponent"),
					__FUNCTION__))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	FVector const DashDirection = FRotator(0.f, StoredPayload.Yaw, 0.f).Vector();
	MovementComponent->RequestDash(DashDirection * DashDistance / DashDuration, DashDuration);
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
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
