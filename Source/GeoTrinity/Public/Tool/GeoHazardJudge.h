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
	/** Duration of a hazard that lasts until EndAt says otherwise, like a zone placed in the level. */
	static constexpr float UntilEnded = TNumericLimits<float>::Max();

	/** What one Judge found for a target that spent time inside the hazard. */
	struct FTargetResult
	{
		AActor* Target = nullptr;
		/** Time the target spent inside since its last judge. */
		float TimeInside = 0.f;
	};

	/**
	 * Forgets every target and starts judging a new hazard.
	 *
	 * @param InStartServerTime          Server time of the hazard's SpentTime 0.
	 * @param InDuration                 How long the hazard stays live from SpentTime 0.
	 * @param bInSeenThroughReplication  True when clients only learn about the hazard through replication (a deployable
	 *                                   exploding), so each one saw it half its ping late; false when they run it on the
	 *                                   server clock themselves (a pattern).
	 * @param bInEndsOnHit               True for a hazard its first hit consumes (a bullet): the hazard ends at the
	 *                                   moment of that hit, so nobody is judged past it — while a target whose own time
	 *                                   is still before it can still be hit by it.
	 * @param bInRemovesInfiniteEffectsOnLeave  True to remove a target's infinite effects when it leaves the hazard;
	 *                                          false keeps them on for good. Only meaningful for a hazard that lasts.
	 */
	void Start(float InStartServerTime, float InDuration, bool bInSeenThroughReplication, bool bInEndsOnHit,
			   bool bInRemovesInfiniteEffectsOnLeave);

	/** Every actor a hazard owned by SourceOwner may hit, damageable ones only. Judge takes the list rather than
	 * building it, so judging many hazards at once — a boss's bullets — walks the world once per tick. */
	static TArray<AActor*> FindCandidates(AActor const* SourceOwner, int32 TeamAttitude);

	/**
	 * For each candidate, runs IsInHazard on every sample its own time has passed since its last judge, at its location
	 * interpolated between the two, then applies Effects: the non per-second ones if it entered, the per-second ones
	 * for the time it spent inside. A target leaves the hazard when a sample finds it outside, when the hazard is over
	 * for it, or when it stops being a candidate; the infinite effects it got on entering go with it, unless
	 * Start said to keep them.
	 * A target seen for the first time, or back after missing a judge, is only judged from now. The last sample sits
	 * exactly on the hazard's end. Does nothing once stopped, and stops for good if Source's owner is gone.
	 *
	 * @param Source      The shot the hazard belongs to: its owner applies Effects and takes the hit credit.
	 * @param Effects     What the hazard applies; may be empty for a hazard that only acts on the result.
	 * @param Candidates  Everything the hazard may hit this tick, from FindCandidates.
	 * @param IsInHazard  Whether Target, standing at Location, is inside the hazard at SpentTime. Called for past
	 *                    moments of each target, so it must derive everything from its arguments.
	 * @return            Every target that spent time inside since its last judge, for a hazard that does more than
	 *                    apply Effects (a zone paying for its heal).
	 */
	TArray<FTargetResult> Judge(FAbilityPayload const& Source, TArray<TInstancedStruct<FEffectData>> const& Effects,
			   TArray<AActor*> const& Candidates,
			   TFunctionRef<bool(AActor const* Target, FVector2D Location, float SpentTime)> IsInHazard);

	/** True once every target has been judged — guaranteed after a fixed window, since GetPerceivedServerTime never
	 * trails the server by more than MaxLatencyCompensation. */
	bool IsOver(float ServerTime) const;

	/** Ends the hazard at EndServerTime if it was to last longer — a hazard stopping before its time, or an endless one
	 * finally stopping. Targets whose own time is still before it go on being judged up to it; calling it again later
	 * never extends it. */
	void EndAt(float EndServerTime);

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

	/** True once SampleIndex comes after the sample sitting on the hazard's end. */
	bool IsPastEnd(int32 SampleIndex) const;

	/** Target's own time on the hazard's timeline: its perceived time, minus how late its screen showed the hazard. */
	float GetTargetSpentTime(AActor const* Target) const;

	/**
	 * Runs IsInHazard on every sample Target's own time has passed since State's last judge, then moves State up to
	 * TargetSpentTime. A hazard that ends on hit ends at the sample that hits.
	 *
	 * @param bOutEntered  Whether a sample found it inside right after one found it outside.
	 * @return             How many samples found it inside.
	 */
	int32 AdvanceSamples(FTargetState& State, AActor const* Target, float TargetSpentTime, FVector2D TargetLocation,
						 TFunctionRef<bool(AActor const* Target, FVector2D Location, float SpentTime)> IsInHazard,
						 bool& bOutEntered);

	/** Applies Effects to Target for what AdvanceSamples found, and removes its infinite effects if it is now outside. */
	void ApplySamplingResult(FAbilityPayload const& Source, TArray<TInstancedStruct<FEffectData>> const& Effects,
							 AActor* Target, FTargetState& State, bool bEntered, float TimeInside);

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
	/** Cut down to the hit's own sample when a hazard that ends on its first hit is hit. */
	float Duration = 0.f;
	bool bSeenThroughReplication = false;
	bool bEndsOnHit = false;
	bool bRemovesInfiniteEffectsOnLeave = false;
	bool bJudging = false;
};
