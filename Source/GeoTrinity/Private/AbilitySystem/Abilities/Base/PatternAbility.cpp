// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/PatternAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArena.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

void UPatternAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	ensureMsgf(PatternToLaunch, TEXT("Please fill the PatternToLaunch in Blueprint"));
	ensureMsgf(GeoLib::IsServer(GetWorld()), TEXT("PatternAbility are made for Server initiated abilities only."));
	if (AnimMontage && !ShouldPlayAbilityMontage())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("%s sets an AnimMontage but its pattern has one too: the ability's is ignored"),
			   *GetName());
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	LaunchSeed = GetNewSeed();
	ActivationParams = MakeActivationParams(TriggerEventData);
	StoredPayload = CreateAbilityPayload(LaunchSeed);

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

bool UPatternAbility::ShouldPlayAbilityMontage() const
{
	return !PatternToLaunch || !GetDefault<UPattern>(PatternToLaunch)->HasMontage();
}

bool UPatternAbility::RequiresArena() const
{
	return Target.Mode == EGeoTarget::ArenaCenter
		|| (PatternToLaunch && GetDefault<UPattern>(PatternToLaunch)->RequiresArena());
}

FPatternAbilityParams UPatternAbility::MakeActivationParams(FGameplayEventData const* TriggerEventData) const
{
	FGameplayAbilityTargetData const* const GivenData =
		TriggerEventData ? TriggerEventData->TargetData.Get(0) : nullptr;
	if (GivenData && GivenData->GetScriptStruct()->IsChildOf(FPatternAbilityParams::StaticStruct()))
	{
		return *static_cast<FPatternAbilityParams const*>(GivenData);
	}

	FPatternAbilityParams CasterArenaParams;
	if (AGeoArena const* const Arena = AGeoArena::GetArenaOfBoss(GetAvatarActorFromActorInfo()))
	{
		CasterArenaParams.ArenaTag = Arena->ArenaTag;
	}

	return CasterArenaParams;
}

TInstancedStruct<FPatternData> UPatternAbility::CreatePatternData() const
{
	return ActivationParams.PatternData;
}

FGeoTargetContext UPatternAbility::MakeTargetContext() const
{
	FGeoTargetContext Context = Super::MakeTargetContext();
	Context.ArenaTag = ActivationParams.ArenaTag;
	return Context;
}

void UPatternAbility::BeginPreLaunch()
{
	Picks = GeoASLib::PickTargets(Target, MakeTargetContext());
	for (FGeoTargetPick const& Pick : Picks)
	{
		AActor* const MarkedActor = Pick.Actor.IsValid() ? Pick.Actor.Get() : GetAvatarActorFromActorInfo();
		AddPreLaunchCue(GeoASLib::GetGeoAscFromActor(MarkedActor));
	}
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
	if (Picks.IsEmpty())
	{
		Picks = GeoASLib::PickTargets(Target, MakeTargetContext());
	}

	TArray<FVector2D> const TargetLocations = GeoASLib::ResolvePicks(Picks);
	Picks.Reset();
	if (TargetLocations.IsEmpty())
	{
		UE_LOG(LogGeoASC, Log, TEXT("%s: no target location, nothing launched"), *GetName());
		OnNoTarget();
	}
	else
	{
		UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
		ASC->PatternStartMulticast(StoredPayload, PatternToLaunch, TargetLocations, CreatePatternData());
		UPattern* PatternInstance = nullptr;
		if (ensureMsgf(ASC->FindPatternByClass(PatternToLaunch, PatternInstance),
					   TEXT("Pattern Instance doesn't exist when launching PatternAbility !")))
		{
			PatternInstance->OnPatternEnd.AddUniqueDynamic(this, &UPatternAbility::OnPatternEnd);
		}
		else
		{
			EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, true);
		}
	}
}

void UPatternAbility::OnNoTarget()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
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

	ActivationParams = FPatternAbilityParams();
	Picks.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
