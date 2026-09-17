// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

class AActor;
struct FAbilityPayload;
struct FEffectData;

/**
 * Server. Lag compensation for a hazard, a volume hitting whoever stands in it over [0, Duration] (an instant when
 * Duration is 0). Each hostile is judged at its own time (GeoLib::GetPerceivedServerTime), SampleRate times per second,
 * so the server holds it where it stood when its screen showed that moment. See the lag compensation rule in
 * AbilitySystem/Abilities/Pattern/CLAUDE.md.
 */
struct GEOTRINITY_API FGeoHazardJudge
{
	/**
	 * Forgets every target and starts judging a new hazard.
	 *
	 * @param InStartServerTime          Server time of the hazard's SpentTime 0.
	 * @param InDuration                 How long the hazard stays live from SpentTime 0.
	 * @param InTeamAttitude             Which attitudes, relative to the source owner's team, the hazard hits.
	 * @param bInSeenThroughReplication  True when clients only learn about the hazard through replication (a deployable
	 *                                   exploding), so each one saw it half its ping late; false when they run it on the
	 *                                   server clock themselves (a pattern).
	 */
	void Start(float InStartServerTime, float InDuration, int32 InTeamAttitude, bool bInSeenThroughReplication);

	/**
	 * For each hostile, runs IsInHazard on every sample its own time has passed since its last judge, at its location
	 * interpolated between the two, then applies Effects: the non per-second ones if it entered, the per-second ones
	 * for the time it spent inside. A target leaves the hazard when a sample finds it outside, when the hazard is over
	 * for it, or when it stops being a candidate; the infinite effects it got on entering go with it.
	 * A target seen for the first time, or back after missing a judge, is only judged from now. The last sample sits
	 * exactly on the hazard's end. Does nothing once stopped, and stops for good if Source's owner is gone.
	 *
	 * @param Source      The shot the hazard belongs to: its owner applies Effects and takes the hit credit.
	 * @param IsInHazard  Whether Target, standing at Location, is inside the hazard at SpentTime. Called for past
	 *                    moments of each target, so it must derive everything from its arguments.
	 */
	void Judge(FAbilityPayload const& Source, TArray<TInstancedStruct<FEffectData>> const& Effects,
			   TFunctionRef<bool(AActor const* Target, FVector2D Location, float SpentTime)> IsInHazard);

	/** True once every target has been judged — guaranteed after a fixed window, since GetPerceivedServerTime never
	 * trails the server by more than MaxLatencyCompensation. */
	bool IsOver(float ServerTime) const;

	/** Stops judging, even midway through Judge, and removes the infinite effects still applied. */
	void Stop();

private:
	/** What the judge knows of one target, kept while it stays a candidate. */
	struct FTargetState
	{
		/** Next sample to judge for this target, counted from SpentTime 0. */
		int32 NextSampleIndex = 0;
		/** The target's own SpentTime and location at the last judge, the start of the next interpolation. */
		float LastTime = 0.f;
		FVector2D LastLocation = FVector2D::ZeroVector;
		/** Whether the last judged sample was inside the hazard. */
		bool bInside = false;
		/** Infinite effects applied when it entered, removed when it leaves. */
		TArray<FActiveGameplayEffectHandle> InfiniteEffectHandles;
	};

	/** Samples per second, whatever the frame rate. */
	static constexpr float SampleRate = 120.f;
	static constexpr float SampleInterval = 1.f / SampleRate;

	/** Target's own time on the hazard's timeline: its perceived time, minus how late its screen showed the hazard. */
	float GetTargetSpentTime(AActor const* Target) const;

	/**
	 * Applies the Effects entries matching bPerSecond to Target and reports the hit.
	 *
	 * @return  Handles of the infinite effects still active on Target after the apply.
	 */
	static TArray<FActiveGameplayEffectHandle> ApplyEffects(FAbilityPayload const& Source,
															TArray<TInstancedStruct<FEffectData>> const& Effects,
															bool bPerSecond, AActor* Target, float PerSecondDuration);

	/** Removes the infinite effects State holds from Target, if they are still active. */
	static void RemoveInfiniteEffects(AActor* Target, FTargetState& State);

	TMap<TWeakObjectPtr<AActor>, FTargetState> Targets;

	float StartServerTime = 0.f;
	float Duration = 0.f;
	int32 TeamAttitude = 0;
	bool bSeenThroughReplication = false;
	bool bJudging = false;
};
