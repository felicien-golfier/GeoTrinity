// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "HUD/Interface/GeoDamageNumberHost.h"
#include "Settings/GameDataSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

#include "GeoDeployableBase.generated.h"


struct FEffectData;
class UWidgetComponent;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeployableDestroyed, AGeoDeployableBase*, Deployable);

/** Configuration parameters set by the deploy ability and passed to the deployable actor via FDeployableData. */
USTRUCT(Blueprintable)
struct FDeployableDataParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BlinkDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float LifeDrainMaxDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Size = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Value = 0.f;
};

/** Runtime init data passed from the spawner projectile to the deployable actor before BeginPlay. */
USTRUCT()
struct FDeployableData : public FInteractableActorData
{
	GENERATED_BODY()

	UPROPERTY(Transient, NotReplicated)
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	UPROPERTY(Transient)
	FDeployableDataParams Params;

	// Tag of the ability that spawned this deployable. Forwarded to ApplyEffectFromEffectData on every effect the
	// deployable applies, so the effect can identify its originating ability (e.g. auto-attack detection in ExecCalc).
	UPROPERTY(Transient)
	FGameplayTag AbilityTag;
};

/**
 * Base class for all deployable actors (turrets, walls, healing zones).
 * Replicated actors — spawned by the server and destroyed when expired or recalled.
 */
UCLASS(Abstract)
class GEOTRINITY_API AGeoDeployableBase : public AGeoInteractableActor, public IGeoDamageNumberHost
{
	GENERATED_BODY()

public:
	/** Creates WidgetAnchorComponent (non-rotating health-bar anchor) and CombattantWidgetComponent (resolved from
	 * GameDataSettings::CombattantWidgetComponentClass via ObjectInitializer; null on dedicated server). */
	AGeoDeployableBase(FObjectInitializer const& ObjectInitializer);

	/**
	 * Temporarily disables blocking collision, root-motion-pushes all overlapping characters outward,
	 * then re-enables blocking collision after a short fixed delay.
	 * Server only — called automatically from InitInteractable when bPushActorsOnSpawn is true.
	 */
	void PushAway();

	/** Registers replicated deployable properties (bActive, bBlinking). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Calls PushAway() on the server when bPushActorsOnSpawn is set, then delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;

	/** Computes DrainMagnitudePerSecond from Params and applies the initial drain GE. Call after data is set. */
	virtual void InitDrain();
	/** Ticks the blink timer state and calls Expire when health reaches zero. */
	virtual void Tick(float DeltaSeconds) override;
	/** Resolves and attaches the CombattantWidgetComponent before components initialize. */
	virtual void PreInitializeComponents() override;
	/** Registers with the instigator's DeployableManagerComponent and calls InitDrain. */
	virtual void BeginPlay() override;

	/** True if registering a new deployable of this class beyond the limit should expire the oldest instead of being
	 * blocked. */
	UFUNCTION(BlueprintCallable)
	bool DestroyOldestWhenLimitReached() const { return bDestroyOldestWhenLimitReached; }
	/** True if this class is exempt from the DeployableManagerComponent's slot/count limit entirely. Class-level (CDO)
	 * property, so every machine agrees without needing SetDeployableInfinitCount to reach clients. Defaults true —
	 * deployable caps are the exception, not the rule (e.g. AGeoBuffPickup overrides to false so the reload buff
	 * shower still evicts its oldest pickup). */
	UFUNCTION(BlueprintCallable)
	bool IsUnlimitedDeploy() const { return bUnlimitedDeploy; }
	/**
	 * Ends this deployable's lifetime. Calls RecallEffect then Expire.
	 * Always use this instead of Expire or Destroy directly — it is the sole valid end-of-life path.
	 * Should be called on the server only; clients receive bActive replication and respond via OnRep_Active.
	 *
	 * @param Value  Scalar forwarded to RecallEffect for effect scaling (e.g. mine power).
	 */
	void Recall(float Value = 0.f);
	/**
	 * Override hook called inside Recall() on the server. Put class-specific end-of-life logic here
	 * (apply effects, call Explode, etc.). Default is a no-op.
	 *
	 * @param Value  Scalar passed from Recall(), e.g. for explosion damage scaling.
	 */
	virtual void RecallEffect(float Value);
	/** Executes the given GameplayCueTag on this actor's ASC with CueParams. Used by OnRep_Active and Recall() to fire
	 * recall/blink cues locally. */
	void ExecuteCue(FGameplayTag const& GameplayCueTag, FGameplayCueParameters const& CueParams) const;

	/**
	 * Sphere-overlaps interactable actors at the deployable's location with radius Params.Size,
	 * then calls ApplyExplodeEffect per target matching ExplodeAttitude. Server only.
	 *
	 * @param Value  Scalar forwarded to ExplodeEffect for damage/effect scaling.
	 */
	void Explode(float const Value);
	/**
	 * Override hook called per valid target inside Explode(). Default applies EffectDataArray to the target.
	 * Server only.
	 *
	 * @param Value  Scalar forwarded from Explode(), used for damage/effect scaling.
	 */
	virtual void ExplodeEffect(float const Value);

	/** Returns health ratio (0..1). Returns 1 if no duration limit. */
	UFUNCTION(BlueprintPure)
	virtual float GetDurationPercent() const;
	/** Starts the pre-expiry blink timer for the given duration in seconds. */
	virtual void StartBlinking();

	/** Called when duration or health reaches zero, when recalled, or when aborted from above. */
	UFUNCTION()
	virtual void Expire(float TimeBeforeDestroy);
	/** Overload that uses the default TimeBeforeDestroyAtExpire delay. */
	virtual void Expire();

	/** Returns false once the deployable is dead / blinking (health or duration reached zero). */
	UFUNCTION(BlueprintPure)
	bool IsActive() const { return bActive; }

	/** Returns true during the pre-expiry blink window (blink timer is running). */
	UFUNCTION(BlueprintPure)
	bool IsBlinking() const;
	/**
	 * Builds and returns the GameplayCue parameters used when firing the spawn cue.
	 * Override to add class-specific source location or effect context.
	 *
	 * @param SoundTag  Optional sound tag forwarded into the cue parameters.
	 */
	virtual FGameplayCueParameters GetSpawnCueParams(FGameplayTag SoundTag);
	/**
	 * Builds and returns the GameplayCue parameters used when firing the pre-expiry blink cue.
	 *
	 * @param SoundTag  Optional sound tag forwarded into the cue parameters.
	 */
	FGameplayCueParameters GetBlinkCueParams(FGameplayTag SoundTag);

	/** Returns gameplay cue parameters at this actor's location (Z raised just above the floor), with the deploying
	 * instigator. */
	virtual FGameplayCueParameters GetGenericCueParams(FGameplayTag SoundTag = FGameplayTag());

	/** Returns the GameplayCue parameters to use when firing the recall cue. */
	virtual FGameplayCueParameters GetRecallCueParams();

	/** True if this deployable is exempt from the hex arena's fall-check recall when its tile is destroyed. */
	bool SurviveOverTheVoid() const { return bSurviveOverTheVoid; }

	UPROPERTY(BlueprintAssignable)
	FOnDeployableDestroyed OnDeployableExpiredEvent;

	// Non-rotating attachment point for the health bar: its relative offset would orbit the deployable as the capsule
	// yaws if attached to the root (mirrors AGeoCharacter::WidgetAnchorComponent).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<USceneComponent> WidgetAnchorComponent;

	// World-space health-bar widget component. Created in the constructor from
	// GameDataSettings::CombattantWidgetComponentClass (the concrete UGeoCombattantWidgetComp lives in the UI module,
	// so gameplay holds it as the engine base). Null on the dedicated server, which does not ship the UI class.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UWidgetComponent> CombattantWidgetComponent;

	// Per-BP health-bar tuning, applied to CombattantWidgetComponent in BeginPlay (the component's own Details panel
	// can't expose these because its class is resolved at runtime). Leave HealthBarWidgetClassOverride null to use the
	// project default from GameDataSettings.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	FVector2D HealthBarDrawSize = FVector2D(80.f, 10.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	FVector HealthBarLocation = FVector(100.f, 0.f, 0.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UUserWidget> HealthBarWidgetClassOverride;

	// When false, the deployable's Health/Shield changes (life drain, incoming damage) spawn no floating combat numbers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	bool bShowDamageNumbers = true;

	/** IGeoDamageNumberHost: gates the HUD's floating-number registration for this deployable. */
	virtual bool ShowsDamageNumbers() const override { return bShowDamageNumbers; }

protected:
	// subclasses MUST override this with their own data struct inherited from FDeployableData
	virtual FDeployableData const* GetData() const override
	{
		checkNoEntry();
		return nullptr;
	}

	virtual void OnHealthChanged_Implementation(float NewValue) override;


	UFUNCTION(BlueprintNativeEvent)
	void OnBlinkStart();
	void OnBlinkStart_Implementation();

	UFUNCTION()
	virtual void OnRep_Active(bool bOldValue);
	UFUNCTION()
	virtual void OnRep_Blinking(bool bOldValue);

	UPROPERTY(BlueprintReadOnly)
	bool bUseRegularDrain = true;
	UPROPERTY(BlueprintReadOnly)
	float DrainMagnitudePerSecond = 0.f;

	UFUNCTION()
	bool HasSpawnGameplayCueTag() const
	{
		return SpawnGameplayCueTag.IsValid()
			&& SpawnGameplayCueTag == GetDefault<UGameDataSettings>()->GenericGameplayCueSoundTag;
	}
	UFUNCTION()
	bool HasRecallGameplayCueTag() const
	{
		return RecallGameplayCueTag.IsValid()
			&& RecallGameplayCueTag == GetDefault<UGameDataSettings>()->GenericGameplayCueSoundTag;
	}
	UFUNCTION()
	bool HasBlinkingGameplayCueTag() const
	{
		return BlinkingGameplayCueTag.IsValid()
			&& BlinkingGameplayCueTag == GetDefault<UGameDataSettings>()->GenericGameplayCueSoundTag;
	}
	UFUNCTION()
	bool HasExplodeGameplayCueTag() const
	{
		return ExplodeGameplayCueTag.IsValid()
			&& ExplodeGameplayCueTag == GetDefault<UGameDataSettings>()->GenericGameplayCueSoundTag;
	}
	UFUNCTION()
	bool HasExpireGameplayCueTag() const
	{
		return ExpireGameplayCueTag.IsValid()
			&& ExpireGameplayCueTag == GetDefault<UGameDataSettings>()->GenericGameplayCueSoundTag;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag SpawnGameplayCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel",
			  meta = (EditCondition = "HasSpawnGameplayCueTag", EditConditionHides, AllowPrivateAccess = true))
	FGameplayTag SpawnSoundTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag RecallGameplayCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel",
			  meta = (EditCondition = "HasRecallGameplayCueTag", EditConditionHides, AllowPrivateAccess = true))
	FGameplayTag RecallSoundTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag BlinkingGameplayCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel",
			  meta = (EditCondition = "HasBlinkingGameplayCueTag", EditConditionHides, AllowPrivateAccess = true))
	FGameplayTag BlinkingSoundTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag ExplodeGameplayCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel",
			  meta = (EditCondition = "HasExplodeGameplayCueTag", EditConditionHides, AllowPrivateAccess = true))
	FGameplayTag ExplodeSoundTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag ExpireGameplayCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel",
			  meta = (EditCondition = "HasExpireGameplayCueTag", EditConditionHides, AllowPrivateAccess = true))
	FGameplayTag ExpireSoundTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	bool bSuppressDrainDamageVisuals = true;


	UPROPERTY(ReplicatedUsing = OnRep_Active)
	bool bActive = true;
	// True only when the deployable ended via Recall(); lets OnRep_Active skip the recall cues on a plain expiry.
	UPROPERTY(Replicated)
	bool bRecalled = false;
	UPROPERTY(ReplicatedUsing = OnRep_Blinking)
	bool bBlinking = false;

	float TimeBeforeDestroyAtExpire = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deployable",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag", AllowPrivateAccess = true))
	int32 ExplodeAttitude = TeamAttitudeMask::HostileOrNeutral;
	/** How a target's own collision radius counts toward the explosion overlap. Automatic = center-only for enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	ETargetOverlapMode ExplodeOverlapMode = ETargetOverlapMode::Automatic;
	// Wether should recall or expire when the deployable ends its life on its own.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	bool bExplodeAtRecall = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	bool bAutoRecallAtEndLife = false;

	/** If true, pushes all damageable interactable actors away on spawn and re-enables blocking collision after
	 * CollisionEnableDelay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	bool bPushActorsOnSpawn = false;
	float const CollisionEnableDelay = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	bool bDestroyOldestWhenLimitReached = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	bool bUnlimitedDeploy = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	bool bCanSacrificeDrain = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	bool bSurviveOverTheVoid = false;

private:
	UFUNCTION()
	void TryRecallOrExpire();

	void OnBlinkVisibilityTick();
	void EnableActorCollision();

	FTimerHandle BlinkTimerHandle;
	FTimerHandle BlinkVisibilityTimerHandle;
	FTimerHandle CollisionEnableTimerHandle;
};
