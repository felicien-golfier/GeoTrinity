// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoPlayerController.h"
using GeoASL = UGeoAbilitySystemLibrary;

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoGameplayAbility::GetAbilityTag() const
{
	return UGeoAbilitySystemLibrary::GetAbilityTagFromAbility(*this);
}
FAbilityPayload UGeoGameplayAbility::CreatePatternPayload(const FTransform& Transform, AActor* Owner,
	AActor* Instigator) const
{
	FAbilityPayload Payload;
	Payload.Owner = Owner;
	Payload.Instigator = Instigator;
	Payload.Origin = FVector2D(Transform.GetLocation());
	Payload.Yaw = Transform.GetRotation().Z;
	Payload.PatternClass = PatternToLaunch;
	Payload.ServerSpawnTime = AGeoPlayerController::GetServerTime(GetWorld());
	Payload.Seed = FMath::Rand32();
	Payload.AbilityLevel = GetAbilityLevel();
	// TODO: optimise AbilityTag : remove from payload and set only once on Pattern Creation.
	Payload.AbilityTag = GetAbilityTag();
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
		const FAbilityPayload& Payload = CreatePatternPayload(Owner->GetTransform(), Owner, Owner);
		GetGeoAbilitySystemComponentFromActorInfo()->PatternStartMulticast(Payload);
	}
}

TArray<FEffectData> UGeoGameplayAbility::GetEffectDataArray() const
{
	TArray<FEffectData> FilledEffectData;
	for (auto EffectDataAsset : EffectDataAssets)
	{
		FilledEffectData.Append(GeoASL::GetEffectDataArray(EffectDataAsset.LoadSynchronous()));
	}

	FilledEffectData.Append(GeoASL::GetEffectDataArray(EffectDataInstances));

	return FilledEffectData;
}
