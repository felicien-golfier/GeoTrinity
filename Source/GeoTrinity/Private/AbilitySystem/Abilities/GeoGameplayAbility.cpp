// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Animation/FireAnimNotify.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/GameplayLibrary.h"
using GeoASL = UGeoAbilitySystemLibrary;
using GL = GameplayLibrary;
// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoGameplayAbility::GetAbilityTag() const
{
	return GeoASL::GetAbilityTagFromAbility(*this);
}

FAbilityPayload UGeoGameplayAbility::CreateAbilityPayload(const FTransform& Transform, AActor* Owner,
														  AActor* Instigator) const
{
	FAbilityPayload Payload;
	Payload.Owner = Owner;
	Payload.Instigator = Instigator;
	Payload.Origin = FVector2D(Transform.GetLocation());
	Payload.Yaw = Transform.GetRotation().Rotator().Yaw;
	Payload.ServerSpawnTime = GL::GetServerTime(GetWorld());
	Payload.Seed = FMath::Rand32();
	Payload.AbilityLevel = GetAbilityLevel();
	Payload.AbilityTag = GetAbilityTag();
	return Payload;
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

void UGeoGameplayAbility::HandleAnimationMontage(const UAnimInstance* AnimInstance,
												 const FGameplayAbilityActivationInfo& ActivationInfo)
{
	ensureMsgf(AnimMontage && AnimInstance, TEXT("No valid AnimMontage or AnimInstance"));
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	FName CurrentSection = ASC->GetCurrentMontageSectionName();
	FName SectionToJumpTo;

	if (!AnimInstance->Montage_IsPlaying(AnimMontage))
	{
		// Case Montage is not playing
		ASC->PlayMontage(this, ActivationInfo, AnimMontage, 1.f);
		SectionToJumpTo = GL::SectionStartName;
	}
	else if (CurrentSection == GL::SectionStartName)
	{
		// Case we are still in Start
		SectionToJumpTo = AnimMontage->GetSectionName(AnimMontage->GetSectionIndex(GL::SectionStartName) + 1);
	}
	else if (CurrentSection == GL::SectionStopName)
	{
		// Case we are already in Stop
		SectionToJumpTo = GL::SectionStartName;
	}
	else
	{
		// Case we are still in a combo, or ending.
		const float SectionTime = ASC->GetCurrentMontageSectionLength() - ASC->GetCurrentMontageSectionTimeLeft();
		constexpr float AcceptableComboTimeThreshold = 0.1f; // Equivalent to 3 frame of delay
		if (CurrentSection.ToString().Contains(GL::SectionEndString) && SectionTime > AcceptableComboTimeThreshold)
		{
			// currently ending combo, and too late to jump to next combo.
			SectionToJumpTo = GL::SectionStartName;
		}
		else
		{
			// Jump to next combo
			const FName FireSection = GL::SectionFireName;
			const FString IndexChar = CurrentSection.ToString().Right(1);
			if (!IndexChar.IsNumeric())
			{
				// Case where we do not have combo with number.
				SectionToJumpTo = AnimMontage->GetSectionName(AnimMontage->GetSectionIndex(GL::SectionStartName) + 1);
			}
			else
			{
				int i = FCString::Atoi(*IndexChar);
				FString NextFire = FireSection.ToString();
				NextFire.AppendInt(++i);
				if (!AnimMontage->IsValidSectionName(FName(NextFire)))
				{
					// Cycle back to 1.
					NextFire = FireSection.ToString();
					NextFire.AppendInt(1);
				}

				SectionToJumpTo = FName(NextFire);
			}
		}
	}

	if (!AnimMontage->IsValidSectionName(SectionToJumpTo))
	{
		UE_LOG(LogGeoASC, Error, TEXT("Section %s doesn't exist ! Fallback to Start."), *SectionToJumpTo.ToString());
		SectionToJumpTo = GL::SectionStartName;
	}

	CurrentSection = SectionToJumpTo;
	ASC->CurrentMontageJumpToSection(SectionToJumpTo);

	float StartTime, EndTime;
	AnimMontage->GetSectionStartAndEndTime(AnimMontage->GetSectionIndex(CurrentSection), StartTime, EndTime);
	float TriggerTime = ASC->GetCurrentMontageSectionTimeLeft();
	for (auto Notify : AnimMontage->Notifies)
	{
		if (Notify.Notify.IsA(UFireAnimNotify::StaticClass()) && Notify.GetTriggerTime() > StartTime
			&& Notify.GetTriggerTime() < EndTime)
		{
			TriggerTime = Notify.GetTriggerTime() - StartTime;
		}
	}

	if (TriggerTime > 0.f)
	{
		if (StartSectionTimerHandle.IsValid())
		{
			UE_LOG(LogGeoASC, Error, TEXT("Set timer with a valid Handle ! Last ability has never ended."));
		}
		GetWorld()->GetTimerManager().SetTimer(StartSectionTimerHandle, this, &UGeoGameplayAbility::AnimTrigger,
											   TriggerTime);
	}
	else
	{
		AnimTrigger();
	}
}

void UGeoGameplayAbility::AnimTrigger()
{
	// Do what you need from the anim.
	StartSectionTimerHandle.Invalidate();
}
