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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Computes DrainMagnitudePerSecond from Params and applies the initial drain GE. Call after data is set. */
	virtual void InitDrain();
	/** Ticks the blink timer state and calls Expire when health reaches zero. */
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/**
	 * Ends this deployable's lifetime. Calls RecallEffect then Expire.
	 * Always use this instead of Expire or Destroy directly — it is the sole valid end-of-life path.
	 * Should be called on the server only; clients receive bActive replication and respond via OnRep_Expired.
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
	/** Fires RecallGameplayCueTag on this actor's ASC. Called by OnRep_Expired on clients when bActive replicates. */
	void ExecuteRecallCue();

	/**
	 * Sphere-overlaps interactable actors at the deployable's location with radius Params.Size,
	 * then calls ExplodeEffect per target matching ExplodeAttitude. Server only.
	 *
	 * @param Value  Scalar forwarded to ExplodeEffect for damage/effect scaling.
	 */
	void Explode(float Value);
	/**
	 * Called once per valid target found by Explode(). Override to change what is applied.
	 * Default applies GetData()->EffectDataArray to the target via ApplyEffectFromEffectData.
	 */
	virtual void ExplodeEffect(float Value, UGeoAbilitySystemComponent* SourceASC, AActor* Actor,
							   UGeoAbilitySystemComponent* TargetASC);

	/** Returns the GameplayCue parameters to use when firing the recall cue. */
	virtual FGameplayCueParameters GetRecallCueParams();

	/** Returns health ratio (0..1). Returns 1 if no duration limit. */
	UFUNCTION(BlueprintPure)
	virtual float GetDurationPercent() const;

	/** Called when duration or health reaches zero, when recalled, or when aborted from above. */
	UFUNCTION()
	virtual void Expire();

	/** Returns true once the deployable has been destroyed (health or duration reached zero). */
	UFUNCTION(BlueprintPure)
	bool IsActive() const { return bActive; }

	/** Returns true during the pre-expiry blink window (blink timer is running). */
	UFUNCTION(BlueprintPure)
	bool IsBlinking() const;

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
	void OnBlinkStarted();
	virtual void OnBlinkStarted_Implementation();

	UFUNCTION()
	virtual void OnRep_Expired(bool bOldValue);

	UPROPERTY(BlueprintReadOnly)
	bool bUseRegularDrain = true;
	UPROPERTY(BlueprintReadOnly)
	float DrainMagnitudePerSecond = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGameplayTag RecallGameplayCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	bool bSuppressDrainDamageVisuals = true;

	UPROPERTY(ReplicatedUsing = OnRep_Expired)
	bool bActive = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (AllowPrivateAccess = true))
	float TimeBeforeDestroyAtExpire = 3.f;
	// Wether should recall or expire when the deployable ends its life on its own.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deployable",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag", AllowPrivateAccess = true))
	int32 ExplodeAttitude = TeamAttitudeMask::Hostile;
	bool bAutoRecallAtEndLife = false;

private:
	void OnBlinkTimerExpired();
	void OnBlinkVisibilityTick();

	FTimerHandle BlinkTimerHandle;
	FTimerHandle BlinkVisibilityTimerHandle;
};
