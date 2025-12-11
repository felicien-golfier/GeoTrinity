// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoPlayerController.h"

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoGameplayAbility::GetAbilityTag() const
{
	return UGeoAbilitySystemLibrary::GetAbilityTagFromAbility(*this);
}
FPatternPayload UGeoGameplayAbility::CreatePatternPayload(const FTransform& Transform, AActor* Owner,
	AActor* Instigator) const
{
	FPatternPayload Payload;
	Payload.Owner = Owner;
	Payload.Instigator = Instigator;
	Payload.Origin = FVector2D(Transform.GetLocation());
	Payload.Yaw = Transform.GetRotation().Z;
	Payload.PatternClass = PatternToLaunch;
	Payload.ServerSpawnTime = AGeoPlayerController::GetServerTime(GetWorld());
	Payload.Seed = FMath::Rand32();
	return Payload;
}

UGeoAbilitySystemComponent* UGeoGameplayAbility::GetGeoAbilitySystemComponentFromActorInfo() const
{
	return CastChecked<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

void UGeoGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (PatternToLaunch)
	{
		AActor* Owner = GetOwningActorFromActorInfo();
		const FPatternPayload& Payload = CreatePatternPayload(Owner->GetTransform(), Owner, Owner);
		GetGeoAbilitySystemComponentFromActorInfo()->PatternStartMulticast(Payload);
	}
}
