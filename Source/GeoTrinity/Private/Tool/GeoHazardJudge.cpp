// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoHazardJudge.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoNetcodeDebug.h"
#include "Tool/UGeoGameplayLibrary.h"

void FGeoHazardJudge::Start(float const InStartServerTime, float const InDuration, bool const bInSeenThroughReplication,
							bool const bInEndsOnHit, bool const bInRemovesInfiniteEffectsOnLeave)
{
	ensureMsgf(
		InDuration > 0.f || !bInRemovesInfiniteEffectsOnLeave,
		TEXT("%hs: an instant hazard cannot remove its infinite effects on leave, they would come off the moment "
			 "they land"),
		__FUNCTION__);

	Targets.Reset();
	StartServerTime = InStartServerTime;
	SetDuration(InDuration);
	bSeenThroughReplication = bInSeenThroughReplication;
	bEndsOnHit = bInEndsOnHit;
	bRemovesInfiniteEffectsOnLeave = bInRemovesInfiniteEffectsOnLeave;
	bJudging = true;
}

TArray<AActor*> FGeoHazardJudge::FindCandidates(AActor const* SourceOwner, int32 const TeamAttitude)
{
	return GeoASLib::GetInteractableActors(SourceOwner, GeoASLib::GetTeamId(SourceOwner), TeamAttitude,
										   /*bMustBeDamageable*/ true);
}

void FGeoHazardJudge::Judge(FAbilityPayload const& Source, TArray<TInstancedStruct<FEffectData>> const& Effects,
							TArray<AActor*> const& Candidates,
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

		FGeoNetcodeDebug::DrawHazardJudge(Target, State->LastLocation, State->LastTime, TargetLocation, TargetSpentTime,
										  State->NextSampleIndex, SampleCount);

		bool bEntered;
		int32 const SamplesInside =
			AdvanceSamples(*State, Target, TargetSpentTime, TargetLocation, IsInHazard, bEntered);
		ApplySamplingResult(Source, Effects, Target, *State, bEntered, SamplesInside);
	}
}

bool FGeoHazardJudge::IsOver(float const ServerTime) const
{
	float const MaxLatencyCompensation = GetDefault<UGameDataSettings>()->MaxLatencyCompensation;
	float const MaxSeenDelay = bSeenThroughReplication ? MaxLatencyCompensation : 0.f;
	return ServerTime >= StartServerTime + Duration + MaxSeenDelay + MaxLatencyCompensation;
}

void FGeoHazardJudge::EndAt(float const EndServerTime)
{
	SetDuration(FMath::Min(Duration, FMath::Max(EndServerTime - StartServerTime, 0.f)));
}

void FGeoHazardJudge::SetDuration(float const InDuration)
{
	Duration = InDuration;
	SampleCount = FMath::CeilToInt(Duration * SampleRate) + 1;
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
	float const SeenDelay = bSeenThroughReplication ? GeoLib::GetReplicationDelay(Target) : 0.f;
	return GeoLib::GetPerceivedServerTime(Target) - SeenDelay - StartServerTime;
}

int32 FGeoHazardJudge::AdvanceSamples(
	FTargetState& State, AActor const* Target, float const TargetSpentTime, FVector2D const TargetLocation,
	TFunctionRef<bool(AActor const* Target, FVector2D Location, float SpentTime)> IsInHazard, bool& bOutEntered)
{
	bOutEntered = false;
	int32 SamplesInside = 0;
	for (; State.NextSampleIndex < SampleCount; ++State.NextSampleIndex)
	{
		float const SampleTime = FMath::Min(State.NextSampleIndex * SampleInterval, Duration);
		if (SampleTime > TargetSpentTime)
		{
			break;
		}

		float const TravelledFraction = (SampleTime - State.LastTime) / (TargetSpentTime - State.LastTime);
		FVector2D const SampleLocation = FMath::Lerp(State.LastLocation, TargetLocation, TravelledFraction);
		bool const bInside = IsInHazard(Target, SampleLocation, SampleTime);
		FGeoNetcodeDebug::DrawHazardSample(Target, SampleLocation, SampleTime, State.NextSampleIndex, bInside);

		if (bInside)
		{
			bOutEntered |= !State.bInside;
			++SamplesInside;
			if (bEndsOnHit)
			{
				// The hazard is spent here: nobody is judged past this sample, and this target has no sample left.
				Duration = SampleTime;
				SampleCount = State.NextSampleIndex + 1;
			}
		}

		State.bInside = bInside;
	}

	if (State.NextSampleIndex >= SampleCount)
	{
		State.bInside = false; // The hazard is over for this target, so it leaves it.
	}

	State.LastTime = TargetSpentTime;
	State.LastLocation = TargetLocation;
	return SamplesInside;
}

void FGeoHazardJudge::ApplySamplingResult(FAbilityPayload const& Source,
										  TArray<TInstancedStruct<FEffectData>> const& Effects, AActor* Target,
										  FTargetState& State, bool const bEntered, int32 const SamplesInside)
{
	if (bEntered)
	{
		RemoveInfiniteEffects(Target, State); // It left and came back since the last judge.
		TArray<FActiveGameplayEffectHandle> const InfiniteEffectHandles =
			ApplyEffects(Source, Effects, /*bPerSecond*/ false, Target, 0.f);
		if (bRemovesInfiniteEffectsOnLeave)
		{
			State.InfiniteEffectHandles = InfiniteEffectHandles;
		}
	}

	if (!State.bInside)
	{
		RemoveInfiniteEffects(Target, State);
	}

	if (bJudging && SamplesInside > 0)
	{
		ApplyEffects(Source, Effects, /*bPerSecond*/ true, Target, SamplesInside * SampleInterval);
	}
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
	if (!ensureMsgf(SourceASC, TEXT("%hs: hazard owner %s has no ASC"), __FUNCTION__, *GetNameSafe(Source.SourceOwner)))
	{
		return {};
	}

	TArray<FActiveGameplayEffectHandle> const AppliedHandles = GeoASLib::ApplyEffectFromEffectData(
		MatchingEffects, SourceASC, TargetASC, Source.AbilityLevel, Source.Seed, Source.AbilityTag, PerSecondDuration);
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
