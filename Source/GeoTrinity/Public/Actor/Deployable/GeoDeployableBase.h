// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Data/GeoCueParam.h"
#include "AbilitySystem/Data/GeoFXMoment.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "HUD/Interface/GeoDamageNumberHost.h"
#include "Settings/GameDataSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/GeoColor.h"
#include "Tool/Team.h"

#include "GeoDeployableBase.generated.h"


class UCharacterMovementComponent;
class AGeoArena;
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

	/** Colour the deployable tints its material with, so one Blueprint can serve every ability that spawns it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoColorParam Color;

	/** Which attitudes, relative to the deployable's own team, it acts on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 Attitude = TeamAttitudeMask::All;
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
	virtual FGameplayCueParameters GetSpawnCueParams() const;
	/** Builds and returns the GameplayCue parameters used when firing the pre-expiry blink cue. */
	FGameplayCueParameters GetBlinkCueParams() const;

	/** Returns Cue's parameters at this actor's location (Z raised just above the floor), causing them from the
	 * deployable itself so a notify can tell the wall apart from whoever deployed it. */
	virtual FGameplayCueParameters GetGenericCueParams(FGeoCueParam const& Cue) const;

	/** Returns the GameplayCue parameters to use when firing the recall cue. */
	virtual FGameplayCueParameters GetRecallCueParams() const;

	/** True if this deployable is exempt from the hex arena's fall-check recall when its tile is destroyed. */
	bool SurviveOverTheVoid() const { return bSurviveOverTheVoid; }

	UPROPERTY(BlueprintAssignable)
	FOnDeployableDestroyed OnDeployableExpiredEvent;

	// Non-rotating attachment point for the health bar: its relative offset would orbit the deployable as the capsule
	// yaws if attached to the root (mirrors AGeoCharacter::WidgetAnchorComponent).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TObjectPtr<USceneComponent> WidgetAnchorComponent;

	// World-space health-bar widget component. Created in the constructor from
	// GameDataSettings::CombattantWidgetComponentClass (the concrete UGeoCombattantWidgetComp lives in the UI module,
	// so gameplay holds it as the engine base). Null on the dedicated server, which does not ship the UI class.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoHUD")
	TObjectPtr<UWidgetComponent> CombattantWidgetComponent;

	// Per-BP health-bar tuning, applied to CombattantWidgetComponent in BeginPlay (the component's own Details panel
	// can't expose these because its class is resolved at runtime). Leave HealthBarWidgetClassOverride null to use the
	// project default from GameDataSettings.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoHUD")
	FVector2D HealthBarDrawSize = FVector2D(80.f, 10.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoHUD")
	FVector HealthBarLocation = FVector(100.f, 0.f, 0.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoHUD")
	TSoftClassPtr<UUserWidget> HealthBarWidgetClassOverride;

	// When false, the deployable's Health/Shield changes (life drain, incoming damage) spawn no floating combat
	// numbers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoHUD")
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
	 * Plays every sound SoundMap maps to SoundType through UGeoSoundRowLibrary — audience-gated on the deploying
	 * instigator, with its instigator-relative volume and attribute-driven pitch. Called on every machine that reaches
	 * the moment, next to that moment's cue. Silent when the map holds no entry for SoundType.
	 */
	void PlaySoundOneShot(EDeployableSoundType SoundType) const;

	/**
	 * The gameplay half of an explode-at-recall: sphere-overlaps interactable actors at Params.Size filtered by
	 * ExplodeAttitude and applies EffectDataArray to each. Called from Recall on the server; the matching cosmetics
	 * live in PlayRecallCosmetics so both machines spell them the same way. Override to change what an explosion does.
	 *
	 * @param Value  Scalar used for damage/effect scaling.
	 */
	virtual void ExplodeEffect(float const Value);

	/** Every mesh drawing this deployable. UWidgetComponent derives from UMeshComponent, so the health bar is filtered
	 * out — it is not part of the deployable's look. */
	TInlineComponentArray<UMeshComponent*> GetVisualMeshComponents() const;

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

	/** The drain as a per-second rate, described once in InitDrain; Tick only passes it the tick's length. */
	FDamageEffectData DrainEffectData;

	// Cue fired at each moment of the deployable's life: tag, palette slot and sound tag per moment.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam SpawnCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam RecallCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam BlinkingCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam ExplodeCue;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	FGeoCueParam ExpireCue;

	/** Sounds played at each moment of the deployable's life, alongside that moment's gameplay cue — which is where a
	 * deployable's VFX comes from, so only the Sounds half of a moment is read here. Every sound of a moment plays
	 * together; a moment with no entry plays nothing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	TMap<EDeployableSoundType, FGeoFXMoment> SoundMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	bool bSuppressDrainDamageVisuals = true;

	/** Palette slot the outline post-process draws this deployable's silhouette in. Written to the custom-depth pass as
	 * a stencil index, which M_DeployableOutline turns back into a color by indexing the palette texture the camera
	 * builds. Override is not a valid choice — it has no palette color for the shader to look up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoGameFeel", meta = (AllowPrivateAccess = true))
	EGeoColor OutlineColor = EGeoColor::Neutral;


	UPROPERTY(ReplicatedUsing = OnRep_Active)
	bool bActive = true;
	// True only when the deployable ended via Recall(); lets OnRep_Active skip the recall cues on a plain expiry.
	UPROPERTY(Replicated)
	bool bRecalled = false;
	UPROPERTY(ReplicatedUsing = OnRep_Blinking)
	bool bBlinking = false;

	float TimeBeforeDestroyAtExpire = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoDeployable",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag", AllowPrivateAccess = true))
	int32 ExplodeAttitude = TeamAttitudeMask::HostileOrNeutral;
	/** How a target's own collision radius counts toward the explosion overlap. Automatic = center-only for enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	ETargetOverlapMode ExplodeOverlapMode = ETargetOverlapMode::Automatic;
	// Wether should recall or expire when the deployable ends its life on its own.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bExplodeAtRecall = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bDamageableDuringBlink = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bAutoRecallAtEndLife = false;

	/** If true, pushes all damageable interactable actors away on spawn and re-enables blocking collision after
	 * CollisionEnableDelay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bPushActorsOnSpawn = false;
	float const CollisionEnableDelay = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bDestroyOldestWhenLimitReached = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bUnlimitedDeploy = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bCanSacrificeDrain = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable", meta = (AllowPrivateAccess = true))
	bool bSurviveOverTheVoid = false;

private:
	/** Fires the recall cue and sound, plus the explode cue and sound when bExplodeAtRecall. The one description of
	 * "what a recall looks like", shared by the server path (Recall) and the client path (OnRep_Active). */
	void PlayRecallCosmetics(float Value);

	/** Where Target should end up when pushed PushDistance away from this deployable: straight outward, or — when that
	 * is blocked — toward FightingArena's fight centre. */
	FVector ComputePushTarget(AActor* Target, float PushDistance, AGeoArena const* FightingArena) const;

	/** Drives Movement from From to To with a short root-motion source, and schedules its removal. */
	void ApplyPushRootMotion(UCharacterMovementComponent* Movement, FVector const& From, FVector const& To);

	UFUNCTION()
	void TryRecallOrExpire();

	/** Renders every mesh of this deployable into the custom-depth pass with OutlineColor's slot index, so the outline
	 * post-process material can draw its silhouette in that color. */
	void ApplyOutlineStencil() const;

	void OnBlinkVisibilityTick();
	void EnableActorCollision();

	FTimerHandle BlinkTimerHandle;
	FTimerHandle BlinkVisibilityTimerHandle;
	FTimerHandle CollisionEnableTimerHandle;
};
