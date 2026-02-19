// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GameplayLibrary.h"
using GeoASL = UGeoAbilitySystemLibrary;
using GL = GameplayLibrary;

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoGameplayAbility::GetAbilityTag() const
{
	return GeoASL::GetAbilityTagFromAbility(*this);
}

FAbilityPayload UGeoGameplayAbility::CreateAbilityPayload(AActor* Owner, AActor* Instigator, FVector2D const& Origin,
														  float Yaw, float ServerSpawnTime, int Seed) const
{
	FAbilityPayload Payload;
	Payload.Owner = Owner;
	Payload.Instigator = Instigator;
	Payload.Origin = Origin;
	Payload.Yaw = Yaw;
	Payload.ServerSpawnTime = ServerSpawnTime;
	Payload.Seed = Seed;
	Payload.AbilityLevel = GetAbilityLevel();
	Payload.AbilityTag = GetAbilityTag();
	return Payload;
}
FAbilityPayload UGeoGameplayAbility::CreateAbilityPayload(AActor* Owner, AActor* Instigator,
														  FTransform const& Transform) const
{
	return CreateAbilityPayload(Owner, Instigator, FVector2D(Transform.GetLocation()),
								Transform.GetRotation().Rotator().Yaw, GL::GetServerTime(GetWorld()), FMath::Rand32());
}

UGeoAbilitySystemComponent* UGeoGameplayAbility::GetGeoAbilitySystemComponentFromActorInfo() const
{
	return CastChecked<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

TArray<TInstancedStruct<FEffectData>> UGeoGameplayAbility::GetEffectDataArray() const
{
	TArray<TInstancedStruct<FEffectData>> FilledEffectData;
	for (auto EffectDataAsset : EffectDataAssets)
	{
		FilledEffectData.Append(GeoASL::GetEffectDataArray(EffectDataAsset.LoadSynchronous()));
	}

	FilledEffectData.Append(EffectDataInstances);

	return FilledEffectData;
}

float UGeoGameplayAbility::GetCooldown(int32 level) const
{
	float cooldown = 0.f;

	UGameplayEffect* pCooldownEffect = GetCooldownGameplayEffect();
	if (!pCooldownEffect)
	{
		return cooldown;
	}

	pCooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(level, cooldown);
	return cooldown;
}

void UGeoGameplayAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
									 FGameplayAbilityActorInfo const* ActorInfo,
									 FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
									 bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	FireTriggerTimerHandle.Invalidate();
}

void UGeoGameplayAbility::ScheduleFireTrigger(FGameplayAbilityActivationInfo const& ActivationInfo,
											  UAnimInstance* AnimInstance, float ClientServerSpawnTime)
{
	bool const bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();

	float TriggerTime = FireRate;

	if (AnimInstance && AnimMontage && !bIsServer && FireRate > 0.f)
	{
		HandleAnimationMontage(AnimInstance, ActivationInfo);
	}

	if (TriggerTime > 0.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTriggerTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(FireTriggerTimerHandle, this, &UGeoGameplayAbility::Fire, FireRate);
	}
	else
	{
		Fire();
	}
}

void UGeoGameplayAbility::HandleAnimationMontage(UAnimInstance* AnimInstance,
												 FGameplayAbilityActivationInfo const& ActivationInfo)
{
	ensureMsgf(AnimMontage && AnimInstance, TEXT("No valid AnimMontage or AnimInstance"));
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	int32& FireSectionIndex = ASC->GetFireSectionIndex(GetAbilityTag());

	if (!AnimInstance->Montage_IsPlaying(AnimMontage))
	{
		ASC->PlayMontage(this, ActivationInfo, AnimMontage, 1.f);
		FireSectionIndex = 0;
	}

	// Build section name: first activation goes to "Start", then cycle Fire1 -> Fire2 -> ... -> Fire1
	FName SectionToJumpTo;
	if (FireSectionIndex == 0)
	{
		SectionToJumpTo = GL::SectionStartName;
	}
	else
	{
		FString FireSectionName = GL::SectionFireString;
		FireSectionName.AppendInt(FireSectionIndex);
		if (!AnimMontage->IsValidSectionName(FName(FireSectionName)))
		{
			FireSectionIndex = 1;
			FireSectionName = GL::SectionFireString;
			FireSectionName.AppendInt(1);
		}
		SectionToJumpTo = FName(FireSectionName);
	}

	if (!AnimMontage->IsValidSectionName(SectionToJumpTo))
	{
		UE_LOG(LogGeoASC, Error, TEXT("Section %s doesn't exist ! Fallback to Start."), *SectionToJumpTo.ToString());
		SectionToJumpTo = GL::SectionStartName;
	}

	ASC->CurrentMontageJumpToSection(SectionToJumpTo);

	// Adjust play rate so the section fits within FireRate
	float StartTime, EndTime;
	AnimMontage->GetSectionStartAndEndTime(AnimMontage->GetSectionIndex(SectionToJumpTo), StartTime, EndTime);
	float const SectionLength = EndTime - StartTime;
	ensureMsgf(SectionLength > 0.f, TEXT("Current section has no length"));

	float const PlayRate = SectionLength / FireRate;
	AnimInstance->Montage_SetPlayRate(AnimMontage, PlayRate);
	FireSectionIndex++;
}

void UGeoGameplayAbility::Fire()
{
	// Do what you need from the ability when the FireRate end is reach
}
