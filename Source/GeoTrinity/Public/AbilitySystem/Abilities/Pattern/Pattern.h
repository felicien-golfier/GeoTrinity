// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once
#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoCueParam.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/GeoHazardJudge.h"
#include "Tool/Team.h"

#include "Pattern.generated.h"

struct FGameplayTagContainer;
class UAnimInstance;
class UEffectDataAsset;
class UGameplayEffect;
class AGeoProjectile;
struct FAbilityPayload;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatternEvent);

/**
 * Base class for all enemy bullet patterns. A pattern is a UObject created per-client by UGeoAbilitySystemComponent
 * in response to a multicast RPC, so it runs identically on every machine. Subclasses override StartPattern or
 * TickPattern to define when and how projectiles are spawned.
 * The tick loop uses server-synchronized time so the pattern is deterministic across all clients despite ping.
 * A pattern that hits through geometry sets bHasHazard and implements IsInHazard; the base judges it.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UPattern : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Called immediately after the pattern is created. AbilityTag is stored for montage section lookup.
	 * Owner is the enemy character that activated the ability; subclasses may access its components here.
	 */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner);
	/** Client-only. Fires Cue at the pattern's zone location(s) via the instigator's ASC. Override to fire
	 * at multiple locations (e.g. one cue per pillar spawn point). */
	virtual void ExecuteGameplayCue(FGeoCueParam const& Cue);
	/** Builds the FGameplayCueParameters Cue is fired with, at the pattern origin. Override to inject custom fields
	 * (magnitude, timing) on top of the base ones. */
	virtual FGameplayCueParameters FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload);

	/**
	 * Stores the payload and triggers the start-section animation before delegating to StartPattern.
	 * PatternData carries optional pattern-specific replicated data; subclasses read their own FPatternData subclass
	 * via PatternData.GetPtr<T>(). Unset for patterns that need no extra data.
	 */
	virtual void InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData);

	/** Returns true while the pattern is running — set on entering InitPattern, cleared on leaving EndPattern.
	 * A pattern that has been created via OnCreate but not yet initialised via InitPattern returns false. */
	bool IsPatternActive() const { return bPatternIsActive; }

	/**
	 * Ends the pattern and cleans up timers. A pattern with a hazard is ended by the base once it is judged; never call
	 * this for its natural end.
	 * When bForceStop is false, jumps the montage to its end section and broadcasts OnPatternEnd.
	 * When bForceStop is true, stops all montages immediately and skips the OnPatternEnd broadcast —
	 * used by PatternAbility::EndAbility to force-end a pattern without re-triggering the ability end chain.
	 */
	UFUNCTION(BlueprintCallable, Category = "GeoPattern")
	virtual void EndPattern(bool bForceStop = false);

	UPROPERTY(BlueprintAssignable)
	FOnPatternEvent OnPatternEnd;
	UPROPERTY(BlueprintAssignable)
	FOnPatternEvent OnPatternStart;

protected:
	// Called when montage start is done and starts the loop.
	UFUNCTION()
	virtual void StartPattern();

	/**
	 * Called each tick once StartPattern ran. DeltaTime is intentionally not provided — all
	 * timing must be derived from SpentTime so the pattern is deterministic across clients.
	 *
	 * @param ServerTime  Synchronized server time (replicated server time minus half ping).
	 * @param SpentTime   ServerTime minus the payload's ServerSpawnTime — elapsed time since pattern start.
	 */
	virtual void TickPattern(float ServerTime, float SpentTime);

	/**
	 * Same loop as TickPattern but for the wind-up preceding StartPattern — override to keep a telegraph following
	 * its target. SpentTime shares TickPattern's timeline, so it is NEGATIVE here, counting up to 0 as the pattern
	 * goes live. Not called on the too-late path, which has no wind-up.
	 */
	virtual void TickDuringInit(float SpentTime /* /!\ SpentTime is NEGATIVE value until 0 when StartPattern */
	);

	/** How long the hazard stays live, from SpentTime 0. The default 0 judges a single instant. */
	virtual float GetHazardDuration() const;

	/** Server. Whether Target, standing at Location, is inside the hazard at SpentTime. Pure geometry: called for
	 * past moments of each target, so it must derive everything from its arguments. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const;

	/** Server. Effects the hazard applies: non per-second entries once per entry into the hazard, per-second entries
	 * for the time spent inside. Infinite ones are removed when the target leaves, unless the hazard is instant. */
	virtual TArray<TInstancedStruct<FEffectData>> const& GetHazardEffects() const;

	/** Called once on every machine when SpentTime passes the hazard duration. Jumps the montage to its end section;
	 * override to stop the hazard's visuals, since the server only ends the pattern later. */
	virtual void OnHazardEnd();

	void JumpMontageToEndSection() const;

	/**
	 * True when this machine should play the pattern's montage: the montage and its anim instance both exist, and this
	 * is not a dedicated server. The test is IsDedicatedServer and not !IsServer because the montage is cosmetic —
	 * every machine that renders this boss must play it, listen-server host included; only a viewport-less server skips
	 * it.
	 */
	bool CanPlayMontageLocally(UAnimInstance const* AnimInstance) const;

	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "GeoPattern")
	FAbilityPayload StoredPayload;

	// Pattern-specific replicated data set by the launching UPatternAbility; unset when the pattern needs none.
	// Read your own FPatternData subclass via StoredPatternData.GetPtr<T>().
	UPROPERTY(Transient)
	TInstancedStruct<FPatternData> StoredPatternData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoPattern",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 TeamAttitude = TeamAttitudeMask::HostileOrNeutral;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoPattern")
	TObjectPtr<UAnimMontage> AnimMontage;

	float StartDelay = 0.f;
	float TravelTime = 0.f;

	bool bPatternIsActive = false;

	/**
	 * Set in the constructor of a pattern implementing IsInHazard; off by default, which skips the hazard entirely.
	 * On, the server judges every hostile at its own time over [0, GetHazardDuration()] (FGeoHazardJudge), and the base
	 * ends the pattern: at the hazard's end on clients, once the judge is over on the server.
	 */
	bool bHasHazard = false;

	// Cue fired when the pattern is created (wind-up telegraph) and when it goes live.
	UPROPERTY(EditDefaultsOnly, Category = "GeoPattern")
	FGeoCueParam InitCue;

	UPROPERTY(EditDefaultsOnly, Category = "GeoPattern")
	FGeoCueParam StartCue;

	FTimerHandle StartSectionTimerHandle;
	FTimerHandle TimeSyncTimerHandle;

private:
	/** Timer callback: reads server time, computes SpentTime, delegates to TickDuringInit or TickPattern, then runs
	 * the hazard. Started once by InitPattern, it runs through the wind-up and the pattern itself until EndPattern. */
	UFUNCTION()
	void CalculateTimeAndTickPattern();

	/** Judges the hazard on the server, fires OnHazardEnd once and ends the pattern once it is over. */
	void TickHazard(float ServerTime, float SpentTime);

	/** Server. Started by InitPattern, stopped by EndPattern. */
	FGeoHazardJudge HazardJudge;

	bool bHazardEnded = false;
};
