// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/PatternAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

void UPatternAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	ensureMsgf(PatternToLaunch, TEXT("Please fill the PatternToLaunch in Blueprint"));
	ensureMsgf(GeoLib::IsServer(GetWorld()), TEXT("PatternAbility are made for Server initiated abilities only."));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	LaunchSeed = GetNewSeed();

	if (PreLaunchDelay > 0.f)
	{
		BeginPreLaunch();
		GetWorld()->GetTimerManager().SetTimer(PreLaunchTimerHandle, this, &UPatternAbility::LaunchPattern,
											   PreLaunchDelay);
	}
	else
	{
		LaunchPattern();
	}
}

void UPatternAbility::BeginPreLaunch()
{
	AddPreLaunchCue(GetGeoAbilitySystemComponentFromActorInfo());
}

void UPatternAbility::AddPreLaunchCue(UGeoAbilitySystemComponent* TargetASC)
{
	if (!PreLaunchCue.CueTag.IsValid() || PreLaunchCueASCs.Contains(TargetASC))
	{
		return;
	}
	AActor const* TargetAvatar = IsValid(TargetASC) ? TargetASC->GetAvatarActor() : nullptr;
	if (!ensureMsgf(IsValid(TargetAvatar), TEXT("PatternAbility %s: pre-launch cue target has no ASC avatar"),
					*GetName()))
	{
		return;
	}

	FGameplayCueParameters CueParams =
		PreLaunchCue.MakeCueParams(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo(),
								   TargetAvatar->GetActorLocation(), GetAbilityLevel(), GetAbilityTag());
	CueParams.RawMagnitude = PreLaunchDelay;
	TargetASC->AddGameplayCue(PreLaunchCue.CueTag, CueParams);
	PreLaunchCueASCs.Add(TargetASC);
}

void UPatternAbility::RemovePreLaunchCues()
{
	for (TWeakObjectPtr<UGeoAbilitySystemComponent> const& CueASC : PreLaunchCueASCs)
	{
		if (CueASC.IsValid())
		{
			CueASC->RemoveGameplayCue(PreLaunchCue.CueTag);
		}
	}
	PreLaunchCueASCs.Reset();
}

void UPatternAbility::LaunchPattern()
{
	RemovePreLaunchCues();
	StoredPayload = CreateAbilityPayload(LaunchSeed);

	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	ASC->PatternStartMulticast(StoredPayload, PatternToLaunch, CreatePatternData());
	UPattern* PatternInstance = nullptr;
	if (!ensureMsgf(ASC->FindPatternByClass(PatternToLaunch, PatternInstance),
					TEXT("Pattern Instance doesn't exist when launching PatternAbility !")))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, true);
		return;
	}
	PatternInstance->OnPatternEnd.AddUniqueDynamic(this, &UPatternAbility::OnPatternEnd);
}

void UPatternAbility::OnPatternEnd()
{
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	UPattern* PatternInstance = nullptr;
	if (!ensureMsgf(ASC->FindPatternByClass(PatternToLaunch, PatternInstance),
					TEXT("Pattern Instance doesn't exist at the end of the pattern on server !")))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, true);
		return;
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
	PatternInstance->OnPatternEnd.RemoveDynamic(this, &UPatternAbility::OnPatternEnd);
}

void UPatternAbility::EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
								 bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(PreLaunchTimerHandle);
	PreLaunchTimerHandle.Invalidate();
	RemovePreLaunchCues();

	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	UPattern* PatternInstance = nullptr;
	if (ensureMsgf(ASC->FindPatternByClass(PatternToLaunch, PatternInstance),
				   TEXT("Pattern Instance doesn't exist at ability end !")))
	{
		PatternInstance->EndPattern(true);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
