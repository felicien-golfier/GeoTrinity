// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDashAbility.h"

#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
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
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(IsValid(MovementComponent) && ASC,
					TEXT("%hs: avatar has no UGeoCharacterMovementComponent or no UGeoAbilitySystemComponent"),
					__FUNCTION__))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	FVector const DashStart = ActorInfo->AvatarActor->GetActorLocation();
	FVector DashDirection = FRotator(0.f, StoredPayload.Yaw, 0.f).Vector();
	float GemDashDistance =
		DashDistance * ASC->GetNumericAttribute(UGeoGemAttributeSet::GetDashDistanceMultiplierAttribute());
	bool const bHasDirection = ActorInfo->AvatarActor->GetVelocity().SizeSquared() >= SMALL_NUMBER;
	ASC->OnDashAiming.Broadcast(DashStart, bHasDirection, DashDirection, GemDashDistance);
	MovementComponent->RequestDash(DashDirection * GemDashDistance / DashDuration, DashDuration);
	ASC->OnDashStarted.Broadcast(DashStart);
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
