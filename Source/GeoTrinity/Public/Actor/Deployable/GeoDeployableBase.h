// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

#include "GeoDeployableBase.generated.h"


struct FEffectData;
class UGeoCombattantWidgetComp;

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
};

/**
 * Base class for all deployable actors (turrets, walls, healing zones).
 * Replicated actors — spawned by the server and destroyed when expired or recalled.
 */
UCLASS(Abstract)
class GEOTRINITY_API AGeoDeployableBase : public AGeoInteractableActor
{
	GENERATED_BODY()

public:
	AGeoDeployableBase();

	/**
	 * Temporarily disables blocking collision, root-motion-pushes all overlapping characters outward,
	 * then re-enables blocking collision after a short fixed delay.
	 * Server only — called automatically from InitInteractable when bPushActorsOnSpawn is true.
	 */
	void PushAway();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Calls PushAway() on the server when bPushActorsOnSpawn is set, then delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;

	/** Computes DrainMagnitudePerSecond from Params and applies the initial drain GE. Call after data is set. */
	virtual void InitDrain();
	/** Ticks the blink timer state and calls Expire when health reaches zero. */
	virtual void Tick(float DeltaSeconds) override;
	virtual void PreInitializeComponents() override;
	/** Registers with the instigator's DeployableManagerComponent and calls InitDrain. */
	virtual void BeginPlay() override;

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
	/** Fires RecallGameplayCueTag on this actor's ASC. Called by OnRep_Active on clients when bActive replicates. */
	void ExecuteCue(FGameplayTag const& GameplayCueTag, FGameplayCueParameters const& CueParams) const;

	/**
	 * Sphere-overlaps interactable actors at the deployable's location with radius Params.Size,
	 * then calls ApplyExplodeEffect per target matching ExplodeAttitude. Server only.
	 *
	 * @param Value  Scalar forwarded to ExplodeEffect for damage/effect scaling.
	 */
	void Explode(float Value);
	/**
	 * Called once per valid target found by Explode(). Override to change what is applied.
	 * Default applies GetData()->EffectDataArray to the target via ApplyEffectFromEffectData.
	 */
	virtual void ApplyExplodeEffect(float Value, UGeoAbilitySystemComponent* SourceASC, AActor* Actor,
									UGeoAbilitySystemComponent* TargetASC);

	/** Returns the GameplayCue parameters to use when firing the recall cue. */
	virtual FGameplayCueParameters GetRecallCueParams();

	/** Returns health ratio (0..1). Returns 1 if no duration limit. */
	UFUNCTION(BlueprintPure)
	virtual float GetDurationPercent() const;
	void StartBlinking(float BlinkDuration);

	/** Called when duration or health reaches zero, when recalled, or when aborted from above. */
	UFUNCTION()
	virtual void Expire();

	/** Returns true once the deployable has been destroyed (health or duration reached zero). */
	UFUNCTION(BlueprintPure)
	bool IsActive() const { return bActive; }

	/** Returns true during the pre-expiry blink window (blink timer is running). */
	UFUNCTION(BlueprintPure)
	bool IsBlinking() const;
	FGameplayCueParameters GetGenericCueParams();

	UPROPERTY(BlueprintAssignable)
	FOnDeployableDestroyed OnDeployableExpiredEvent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoCombattantWidgetComp> HealthBarComponent;

protected:
	// subclasses MUST override this with their own data struct inherited from FDeployableData
	virtual FDeployableData const* GetData() const override
	{
		checkNoEntry();
		return nullptr;
	}

	virtual void OnHealthChanged_Implementation(float NewValue) override;

	UFUNCTION(BlueprintNativeEvent)
	void OnBlinkVisualStarted();
	virtual void OnBlinkVisualStarted_Implementation();

	UFUNCTION()
	virtual void OnRep_Active(bool bOldValue);
	UFUNCTION()
	virtual void OnRep_Blinking(bool bOldValue);

	UPROPERTY(BlueprintReadOnly)
	bool bUseRegularDrain = true;
	UPROPERTY(BlueprintReadOnly)
	float DrainMagnitudePerSecond = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag RecallGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag BlinkingGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag ExplodeGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	bool bSuppressDrainDamageVisuals = true;

	UPROPERTY(ReplicatedUsing = OnRep_Active)
	bool bActive = true;
	UPROPERTY(ReplicatedUsing = OnRep_Blinking)
	bool bBlinking = false;

	float TimeBeforeDestroyAtExpire = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deployable",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag", AllowPrivateAccess = true))
	int32 ExplodeAttitude = TeamAttitudeMask::HostileOrNeutral;
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

private:
	UFUNCTION()
	void TryRecallOrExpire();

	void OnBlinkVisibilityTick();
	void EnableActorCollision();

	FTimerHandle BlinkTimerHandle;
	FTimerHandle BlinkVisibilityTimerHandle;
	FTimerHandle CollisionEnableTimerHandle;
};
