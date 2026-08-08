// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Data/GeoCueParam.h"
#include "AbilitySystem/Data/GeoSoundRow.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "HUD/Interface/GeoDamageNumberHost.h"
#include "Settings/GameDataSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/GeoColor.h"
#include "Tool/Team.h"

#include "GeoDeployableBase.generated.h"


struct FEffectData;
class UMeshComponent;
class UWidgetComponent;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeployableDestroyed, AGeoDeployableBase*, Deployable);

/** Moment of a deployable's life a sound plays at. Key of AGeoDeployableBase::SoundMap. */
UENUM(BlueprintType)
enum class EDeployableSoundType : uint8
{
	Spawn,
	Blinking,
	Recall,
	Explode,
	Expire,
	ExpireButNotRecalled
};

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
class GEOTRINITY_API AGeoDeployableBase
	: public AGeoInteractableActor
	, public IGeoDamageNumberHost
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

	/** Executes Cue's tag on this actor's ASC with CueParams. Used by OnRep_Active and Recall() to fire recall/blink
	 * cues locally. */
	void ExecuteCue(FGeoCueParam const& Cue, FGameplayCueParameters const& CueParams) const;

	/** Returns health ratio (0..1). Returns 1 if no duration limit. */
	UFUNCTION(BlueprintPure)
	virtual float GetDrainDurationRatio() const;
	/** Starts the pre-expiry blink timer for the given duration in seconds. */
	virtual void StartBlinking();

	/** Called when duration or health reaches zero, when recalled, or when aborted from above. */
	UFUNCTION()
	virtual void Expire(bool bForce = false);

	/** Returns false once the deployable is dead / blinking (health or duration reached zero). */
	UFUNCTION(BlueprintPure)
	bool IsActive() const { return bActive; }

	/** Returns true during the pre-expiry blink window (blink timer is running). */
	UFUNCTION(BlueprintPure)
	bool IsBlinking() const;
	/**
	 * Builds and returns the GameplayCue parameters used when firing the spawn cue.
	 * Override to add class-specific source location or effect context.
	 */
	virtual FGameplayCueParameters GetSpawnCueParams();
	/** Builds and returns the GameplayCue parameters used when firing the pre-expiry blink cue. */
	FGameplayCueParameters GetBlinkCueParams();

	/** Returns gameplay cue parameters at this actor's location (Z raised just above the floor), with the deploying
	 * instigator and Cue's own color/sound fields filled in. */
	virtual FGameplayCueParameters GetGenericCueParams(FGeoCueParam const& Cue);

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

	// When false, the deployable's Health/Shield changes (life drain, incoming damage) spawn no floating combat
	// numbers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	bool bShowDamageNumbers = true;

	/** IGeoDamageNumberHost: gates the HUD's floating-number registration for this deployable. */
	virtual bool ShowsDamageNumbers() const override { return bShowDamageNumbers; }

protected:
	/**
	 * Returns the deployable's data block.
	 * @warning Subclasses must override and return their own FDeployableData-derived struct. This base implementation
	 * asserts.
	 */
	virtual FDeployableData const* GetData() const override
	{
		checkNoEntry();
		return nullptr;
	}

	/** Triggers StartBlinking when health reaches zero. */
	virtual void OnHealthChanged_Implementation(float NewValue) override;


	/** Blueprint hook fired when the pre-expiry blink window starts — override to play blink visuals (shake, tint,
	 * etc.). */
	UFUNCTION(BlueprintNativeEvent)
	void OnBlinkStart();
	void OnBlinkStart_Implementation();

	/**
	 * Plays the SoundMap entry for SoundType through UGeoSoundRowLibrary — audience-gated on the deploying instigator,
	 * with its instigator-relative volume and attribute-driven pitch. Called on every machine that reaches the moment,
	 * next to that moment's cue. Silent when the map holds no entry for SoundType.
	 */
	void PlaySoundOneShot(EDeployableSoundType SoundType) const;

	/**
	 * Override hook called per valid target inside Explode(). Default applies EffectDataArray to the target.
	 * Server only.
	 *
	 * @param Value  Scalar forwarded from Explode(), used for damage/effect scaling.
	 */
	virtual void ExplodeEffect(float const Value);

	/** Fires the recall or expiry gameplay cue on clients when bActive becomes false. */
	UFUNCTION()
	virtual void OnRep_Active(bool bOldValue);
	/** Fires the blink gameplay cue on clients when bBlinking becomes true. */
	UFUNCTION()
	virtual void OnRep_Blinking(bool bOldValue);

	UPROPERTY(BlueprintReadOnly)
	bool bUseRegularDrain = true;
	UPROPERTY(BlueprintReadOnly)
	float DrainMagnitudePerSecond = 0.f;

	// Cue fired at each moment of the deployable's life: tag, palette slot and sound tag per moment.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam SpawnCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam RecallCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam BlinkingCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam ExplodeCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam ExpireCue;

	/** Sound played at each moment of the deployable's life, alongside that moment's gameplay cue. Sound asset, volume,
	 * audience and attribute-driven pitch per entry; a moment with no entry plays nothing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	TMap<EDeployableSoundType, FGeoSoundEntry> SoundMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	bool bSuppressDrainDamageVisuals = true;

	/** Palette slot the outline post-process draws this deployable's silhouette in. Written to the custom-depth pass as
	 * a stencil index, which M_DeployableOutline turns back into a color by indexing the palette texture the camera
	 * builds. Override is not a valid choice — it has no palette color for the shader to look up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFeel", meta = (AllowPrivateAccess = true))
	EGeoColor OutlineColor = EGeoColor::Neutral;


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
	bool bDamageableDuringBlink = false;
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
	/**
	 * Sphere-overlaps interactable actors at the deployable's location with radius Params.Size,
	 * then calls ApplyExplodeEffect per target matching ExplodeAttitude. Server only.
	 *
	 * @param Value  Scalar forwarded to ExplodeEffect for damage/effect scaling.
	 */
	void Explode(float const Value);

	UFUNCTION()
	void TryRecallOrExpire();

	/** Renders every mesh of this deployable into the custom-depth pass with OutlineColor's slot index, so the outline
	 * post-process material can draw its silhouette in that color. */
	void ApplyOutlineStencil() const;

	/** Every mesh drawing this deployable. UWidgetComponent derives from UMeshComponent, so the health bar is filtered
	 * out — it is not part of the deployable's look. */
	TInlineComponentArray<UMeshComponent*> GetVisualMeshComponents() const;

	void OnBlinkVisibilityTick();
	void EnableActorCollision();

	FTimerHandle BlinkTimerHandle;
	FTimerHandle BlinkVisibilityTimerHandle;
	FTimerHandle CollisionEnableTimerHandle;
};
