// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoHazardJudge.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void FGeoHazardJudge::Start(float const InStartServerTime, float const InDuration, int32 const InTeamAttitude,
							bool const bInSeenThroughReplication)
{
	Targets.Reset();
	StartServerTime = InStartServerTime;
	Duration = InDuration;
	TeamAttitude = InTeamAttitude;
	bSeenThroughReplication = bInSeenThroughReplication;
	bJudging = true;
}

void FGeoHazardJudge::Judge(FAbilityPayload const& Source, TArray<TInstancedStruct<FEffectData>> const& Effects,
							TFunctionRef<bool(AActor const* Target, FVector2D Location, float SpentTime)> IsInHazard)
{
	if (!bJudging)
	{
		return;
	}
	if (!IsValid(Source.SourceOwner))
	{
		UE_LOG(LogGeoTrinity, Log, TEXT("%hs: owner of %s gone before every target of its hazard was judged"),
			   __FUNCTION__, *GetNameSafe(Source.SourceAvatar));
		Stop();
		return;
	}

	int32 const SampleCount = FMath::CeilToInt(Duration * SampleRate) + 1;
	TArray<AActor*> const Candidates = GeoASLib::GetInteractableActors(
		Source.SourceOwner, GeoASLib::GetTeamId(Source.SourceOwner), TeamAttitude, /*bMustBeDamageable*/ true);

	for (auto It = Targets.CreateIterator(); It; ++It)
	{
		AActor* const Target = It->Key.Get();
		if (!Candidates.Contains(Target))
		{
			RemoveInfiniteEffects(Target, It->Value);
			It.RemoveCurrent();
		}
	}

	// bJudging drops when an applied effect ends the hazard, e.g. by killing the last player.
	for (int32 CandidateIndex = 0; bJudging && CandidateIndex < Candidates.Num(); ++CandidateIndex)
	{
		AActor* const Target = Candidates[CandidateIndex];
		float const TargetSpentTime = GetTargetSpentTime(Target);
		FVector2D const TargetLocation(Target->GetActorLocation());

		FTargetState* State = Targets.Find(Target);
		if (!State)
		{
			// As if it had stood here since the previous sample, so only the current one is judged.
			int32 const CurrentSampleIndex = FMath::FloorToInt(TargetSpentTime * SampleRate);
			State = &Targets.Add(Target);
			State->NextSampleIndex = FMath::Max(CurrentSampleIndex, 0);
			State->LastTime = (CurrentSampleIndex - 1) * SampleInterval;
			State->LastLocation = TargetLocation;
		}

		bool bEntered = false;
		int32 SamplesInside = 0;
		for (; State->NextSampleIndex < SampleCount; ++State->NextSampleIndex)
		{
			float const SampleTime = FMath::Min(State->NextSampleIndex * SampleInterval, Duration);
			if (SampleTime > TargetSpentTime)
			{
				break;
			}

			float const TravelledFraction = (SampleTime - State->LastTime) / (TargetSpentTime - State->LastTime);
			FVector2D const SampleLocation = FMath::Lerp(State->LastLocation, TargetLocation, TravelledFraction);
			bool const bInside = IsInHazard(Target, SampleLocation, SampleTime);
			if (bInside)
			{
				bEntered |= !State->bInside;
				++SamplesInside;
			}
			State->bInside = bInside;
		}

		if (State->NextSampleIndex >= SampleCount)
		{
			State->bInside = false; // The hazard is over for this target, so it leaves it.
		}
		State->LastTime = TargetSpentTime;
		State->LastLocation = TargetLocation;

		if (bEntered)
		{
			RemoveInfiniteEffects(Target, *State); // It left and came back since the last judge.
			State->InfiniteEffectHandles = ApplyEffects(Source, Effects, /*bPerSecond*/ false, Target, 0.f);
		}
		if (!State->bInside || !bJudging)
		{
			RemoveInfiniteEffects(Target, *State);
		}
		if (bJudging && SamplesInside > 0)
		{
			ApplyEffects(Source, Effects, /*bPerSecond*/ true, Target, SamplesInside * SampleInterval);
		}
	}
}

bool FGeoHazardJudge::IsOver(float const ServerTime) const
{
	float const MaxLatencyCompensation = GetDefault<UGameDataSettings>()->MaxLatencyCompensation;
	float const MaxSeenDelay = bSeenThroughReplication ? MaxLatencyCompensation : 0.f;
	return ServerTime >= StartServerTime + Duration + MaxSeenDelay + MaxLatencyCompensation;
}

void FGeoHazardJudge::Stop()
{
	bJudging = false;
	for (auto& [Target, State] : Targets)
	{
		RemoveInfiniteEffects(Target.Get(), State);
	}
}

float FGeoHazardJudge::GetTargetSpentTime(AActor const* Target) const
{
	float const PerceivedSpentTime = GeoLib::GetPerceivedServerTime(Target) - StartServerTime;
	APawn const* const Pawn = Cast<APawn>(Target);
	if (!bSeenThroughReplication || !IsValid(Pawn) || !Pawn->IsPlayerControlled() || Pawn->IsLocallyControlled())
	{
		return PerceivedSpentTime;
	}

	float const HalfPing = Pawn->GetPlayerState()->GetPingInMilliseconds() * 0.0005f;
	return PerceivedSpentTime - FMath::Min(HalfPing, GetDefault<UGameDataSettings>()->MaxLatencyCompensation);
}

TArray<FActiveGameplayEffectHandle> FGeoHazardJudge::ApplyEffects(FAbilityPayload const& Source,
																   TArray<TInstancedStruct<FEffectData>> const& Effects,
																   bool const bPerSecond, AActor* Target,
																   float const PerSecondDuration)
{
	TArray<TInstancedStruct<FEffectData>> const MatchingEffects = Effects.FilterByPredicate(
		[bPerSecond](TInstancedStruct<FEffectData> const& Effect)
		{
			FEffectData const* const EffectData = Effect.GetPtr();
			return EffectData && EffectData->IsPerSecond() == bPerSecond;
		});

	UGeoAbilitySystemComponent* const TargetASC = GeoASLib::GetGeoAscFromActor(Target);
	if (MatchingEffects.IsEmpty() || !IsValid(TargetASC))
	{
		return {};
	}

	UGeoAbilitySystemComponent* const SourceASC = GeoASLib::GetGeoAscFromActor(Source.SourceOwner);
	if (!ensureMsgf(SourceASC, TEXT("%hs: hazard owner %s has no ASC"), __FUNCTION__,
					*GetNameSafe(Source.SourceOwner)))
	{
		return {};
	}

	TArray<FActiveGameplayEffectHandle> const AppliedHandles =
		GeoASLib::ApplyEffectFromEffectData(MatchingEffects, SourceASC, TargetASC, Source.AbilityLevel, Source.Seed,
											Source.AbilityTag, PerSecondDuration);
	GeoASLib::NotifyAbilityHit(Source, Target);

	return AppliedHandles.FilterByPredicate(
		[TargetASC](FActiveGameplayEffectHandle const& Handle)
		{
			FActiveGameplayEffect const* const ActiveEffect = TargetASC->GetActiveGameplayEffect(Handle);
			return ActiveEffect && ActiveEffect->GetDuration() == FGameplayEffectConstants::INFINITE_DURATION;
		});
}

void FGeoHazardJudge::RemoveInfiniteEffects(AActor* Target, FTargetState& State)
{
	if (UGeoAbilitySystemComponent* const TargetASC = GeoASLib::GetGeoAscFromActor(Target))
	{
		for (FActiveGameplayEffectHandle const& Handle : State.InfiniteEffectHandles)
		{
			if (TargetASC->GetActiveGameplayEffect(Handle))
			{
				TargetASC->RemoveActiveGameplayEffect(Handle, /*StacksToRemove*/ 1);
			}
		}
	}
	State.InfiniteEffectHandles.Reset();
}
