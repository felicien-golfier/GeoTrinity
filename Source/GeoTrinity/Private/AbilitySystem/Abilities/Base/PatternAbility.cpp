// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/PatternAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Actor/Arena/GeoArena.h"
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

FVector2D UPatternAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
										   int const Seed) const
{
	if (!TargetPointTag.IsValid())
	{
		return Super::GetFireOrigin2D(Instigator, SourceASC, Seed);
	}

	AGeoArena const* const Arena = AGeoArena::GetArenaOfBoss(Instigator);
	if (!ensureMsgf(Arena, TEXT("%hs: %s was not spawned by an arena, so it has no points to launch from"),
					__FUNCTION__, *GetNameSafe(Instigator)))
	{
		return Super::GetFireOrigin2D(Instigator, SourceASC, Seed);
	}

	TArray<AActor*> const TargetPoints = GeoLib::GetTargetPoints(Instigator, TargetPointTag, Arena->ArenaTag);
	if (!ensureMsgf(!TargetPoints.IsEmpty(), TEXT("%hs: no AGeoTargetPoint tagged %s in arena %s"), __FUNCTION__,
					*TargetPointTag.ToString(), *Arena->ArenaTag.ToString()))
	{
		return Super::GetFireOrigin2D(Instigator, SourceASC, Seed);
	}

	return FVector2D(TargetPoints[0]->GetActorLocation());
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
