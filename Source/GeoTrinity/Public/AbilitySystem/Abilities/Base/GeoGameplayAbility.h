// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoGameplayAbility.generated.h"


struct FGeoAbilityTargetData;
struct FAbilityPayload;
struct FExternalProjectileParams;
class UEffectDataAsset;
class UGeoAbilitySystemComponent;
class UPattern;
class APlayableCharacter;
enum class EProjectileTarget : uint8;

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	ShootAfterFireDelay,
	ChargeForFireDelay,
};

UENUM(BlueprintType)
enum class ECommitBehaviour : uint8
{
	AtActivate,
	CostAtActivateCooldownAtEnd,
	DoNotAutoCommit
};

/**
 * Base gameplay ability for GeoTrinity. Provides shared fire-trigger scheduling, animation montage handling,
 * client-to-server target data dispatch, and effect data aggregation used by all derived ability types.
 */
UCLASS()
class GEOTRINITY_API UGeoGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * Commits cost and cooldown, populates StoredPayload from TriggerEventData (or avatar state for passives),
	 * binds the server-side OnFireTargetDataReceived delegate, and schedules the fire trigger.
	 */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Blocks activation while the avatar character is dead. Evaluated by both client prediction and server
	 * validation, so a press sent during the death replication window is rejected and rolled back cleanly. */
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
									FGameplayTagContainer const* SourceTags = nullptr,
									FGameplayTagContainer const* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Returns the primary gameplay tag identifying this ability (first owned tag under "Ability"). */
	FGameplayTag GetAbilityTag() const;

	/**
	 * Constructs an FAbilityPayload from explicit shot parameters.
	 *
	 * @param Origin            2D world position of the fire socket at the moment of firing.
	 * @param Yaw               Character facing yaw at the moment of firing (degrees).
	 * @param ServerSpawnTime   Synchronized server time at which the projectile was spawned.
	 * @param Seed              Random seed for deterministic effects.
	 */
	FAbilityPayload CreateAbilityPayload(FVector2D const& Origin, float Yaw, float ServerSpawnTime, int Seed) const;
	/** Constructs an FAbilityPayload from the current avatar's world state (position, facing, server time, new seed).
	 */
	FAbilityPayload CreateAbilityPayload() const;
	/** Reconstructs an FAbilityPayload from replicated target data received by the server. */
	FAbilityPayload CreateAbilityPayloadFromTargetData(FGeoAbilityTargetData const& TargetData) const;

	/** Returns the GeoAbilitySystemComponent from the ability's current actor info. */
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponentFromActorInfo() const;

	/** Merges EffectDataAssets and EffectDataInstances into a single flat array for application. */
	virtual TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const;

	/**
	 * Returns the cooldown duration in seconds for the given ability level.
	 *
	 * @param level  Ability level to evaluate the cooldown at. Defaults to 1.
	 */
	float GetCooldown(int32 level = 1) const;

	/** Cancels the pending fire timer and hides the charge gauge (ChargeForFireDelay mode) before calling Super. */
	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Shows or hides the charge gauge on the owning PlayableCharacter. Base implementation is client-only (no-op on
	 * server). Override to use a different widget. */
	virtual void SetChargeGaugeVisible(APlayableCharacter* Character, bool bVisible);

	/** Convenience overload that ends this ability instance without requiring handle/actorinfo parameters. */
	void EndAbility(bool bReplicateEndAbility = true, bool bWasCancelled = false);

	/**
	 * Returns the normalized charge progress (0–1) based on time held since activation.
	 * Only meaningful when FireMode == EFireMode::ChargeForFireDelay.
	 */
	float GetChargeRatio() const;
	/**
	 * In ChargeForFireDelay mode, fires immediately on input release instead of waiting for the timer to expire.
	 * Also jumps the montage to SectionEndName on the locally controlled player to prevent the charge animation
	 * from holding its last frame.
	 */
	virtual void InputReleased(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayAbilityActivationInfo ActivationInfo) override;
	/**
	 * Second input whose *press* releases this ability while it runs, on top of its own input's release. Lets a charge
	 * fire on a button press instead of on a key-up. Invalid by default — abilities opt in by overriding.
	 */
	virtual FGameplayTag GetAlternateReleaseInputTag() const { return FGameplayTag{}; }
	/** Returns the effective fire delay: reads GeneralChargeTime from GameDataSettings when
	 * bUseGeneralChargeTimeForFireDelay is set, otherwise uses the per-ability FireDelay. */
	UFUNCTION(BlueprintCallable)
	float GetFireDelay() const;

	/** Returns true if this ability carries the Ability.Type.Passive asset tag. */
	bool IsPassive() const;

	/**
	 * Returns the 2D world-space origin for the projectile spawn, sampled at fire time.
	 * Default reads "anim_socket_<FireSectionIndex>" from the character mesh; falls back to actor location.
	 * Override to change the spawn point (different socket, fixed offset, etc.).
	 */
	virtual FVector2D GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC, int Seed) const;
	/**
	 * 3D variant of GetFireOrigin2D. Default elevates the 2D origin to ArbitraryCharacterZ.
	 * Override for abilities that need a true 3D spawn point.
	 */
	virtual FVector GetFireOrigin(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC, int Seed) const;
	/** Returns the server world time stamped into the payload as ServerSpawnTime. Override to pre-adjust for latency.
	 */
	virtual float GetStartTime(UWorld const* World) const;
	/** Returns a fresh random seed for the next shot. Override for deterministic seed strategies. */
	virtual int GetNewSeed() const;
	/** Returns the instigator's facing yaw in degrees. Override to aim toward a specific target instead. */
	virtual float GetFireYaw(AActor const* Instigator, int Seed) const;
	/** Applies the designer-tunable GaugeChargingSpeedCurve easing to a raw charge ratio in [0, 1]. */
	float ApplyChargingCurve(float RawRatio) const;

	/** Timer callback that calls BuildAbilityTargetData, sends it to the server, and calls Fire on the client. Public
	 * so subclasses can bind it directly to timer delegates. */
	UFUNCTION()
	void BuildDataAndFire();

	/**
	 * Replays this ability's visible shot on a machine that owns no instance of it — a client watching an ally, whose
	 * ActivatableAbilities never replicate. Driven by UGeoAbilitySystemComponent off RemoteFireTag, purely cosmetic:
	 * projectiles spawned here are client-side and apply no effects.
	 *
	 * Called on the class default object, so it must read nothing but its EditDefaultsOnly fields and its arguments:
	 * GetWorld(), GetCurrentActorInfo() and StoredPayload are all invalid on a CDO, and the CDO is shared by every
	 * ally on the machine.
	 *
	 * @param Avatar     The ally character that fired, source of the origin and yaw.
	 * @param SourceASC  The ally's ability system component.
	 */
	virtual void RemoteFireShot(AActor* Avatar, UGeoAbilitySystemComponent* SourceASC) const;

	/** True when RemoteFireShot repeats every GetFireDelay() until RemoteFireTag is removed (hold-to-fire abilities).
	 */
	virtual bool IsRemoteFireLooping() const { return false; }

	/**
	 * Reads back the fire section Avatar's montage is playing right now, so the fire socket can be picked without an
	 * ability instance to advance the counter — the case on a client watching an ally. Returns 0, the "Start" socket,
	 * whenever no numbered fire section is playing. CDO-safe, same constraints as RemoteFireShot.
	 */
	int32 GetPlayingFireSectionIndex(AActor const* Avatar) const;

protected:
	/**
	 * Builds a cosmetic payload from Avatar's replicated state and spawns Params' projectiles along Target's
	 * directions. Shared body of the RemoteFireShot overrides — same CDO-only constraints apply.
	 */
	void SpawnRemoteProjectiles(AActor* Avatar, UGeoAbilitySystemComponent* SourceASC,
								FExternalProjectileParams const& Params, EProjectileTarget Target) const;

	/**
	 * Starts a timer (or charge window) after which BuildDataAndFire is called.
	 * Duration is FireDelay for ShootAfterFireDelay mode, or unbounded for charge mode (fires on input release).
	 */
	void ScheduleFireTrigger(FGameplayAbilityActivationInfo const& ActivationInfo, UAnimInstance* AnimInstance);

	/**
	 * Selects or advances the animation montage's fire section index.
	 * Override in subclasses that cycle through multiple fire animations.
	 */
	virtual int32& GetFireSectionIndex(UGeoAbilitySystemComponent* ASC, UAnimInstance const* AnimInstance);
	/** Plays AnimMontage and jumps to the correct section based on the current fire section index. */
	void HandleAnimationMontage(UAnimInstance* AnimInstance, FGameplayAbilityActivationInfo const& ActivationInfo);
	/** Sends AbilityTargetData to the server via ServerSetReplicatedTargetData for authoritative shot execution. */
	void SendFireDataToServer(FGeoAbilityTargetData const& AbilityTargetData) const;

	/** Client-side shot logic (spawn predicted projectile, play VFX). Override in subclasses. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData);

	/**
	 * Server-side shot logic. Called when the server receives target data from the client.
	 * Override to spawn the authoritative projectile or apply server-side effects.
	 */
	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag);

	/** Builds the FGeoAbilityTargetData for the current shot from character position and facing. Override to customize.
	 */
	virtual FGeoAbilityTargetData GetUpdatedTargetData();
	/** Refreshes Seed, ServerSpawnTime, Origin, and Yaw in StoredPayload from the server's received TargetData
	 * snapshot. */
	void UpdatePayloadFromTargetData(FGeoAbilityTargetData const& TargetData);

	/** Server-side: snaps StoredPayload.Origin back to the fire socket when a remote client reports an origin
	 *  further than GameDataSettings::MaxFireOriginDeviation from its avatar. */
	void ClampRemoteClientOrigin();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> AnimMontage;

	// Define the way the ability start to Fire. After a charge time or directly after FireDelay.
	// When FireMode = Charge, we use FireDelay as max charge time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EFireMode FireMode = EFireMode::ShootAfterFireDelay;

	/** When true, only a fresh button press can activate this ability — the per-frame Held input never does.
	 * Used by abilities sharing an input with another ability (sacrifice channel/detonate) so a held button
	 * cannot chain-activate the counterpart the frame after the first one starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	bool bActivateOnFreshPressOnly = false;

	/**
	 * Loose tag the server holds on the owner's ASC for this ability's whole run, replicated to every client so
	 * non-owning ones can drive RemoteFireShot. Leave invalid on abilities whose visuals already reach other clients
	 * (replicated actors, gameplay cues, patterns) — only ability-spawned client-side projectiles need it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (Categories = "Ability.Remote"))
	FGameplayTag RemoteFireTag;


protected:
	FAbilityPayload StoredPayload;
	FTimerHandle FireTriggerTimerHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	ECommitBehaviour CommitBehaviour = ECommitBehaviour::AtActivate;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	bool bUseGeneralChargeTimeForFireDelay = true;

	// We consider the ability to Fire at the end of the FireDelay delay.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability",
			  meta = (EditCondition = "!bUseGeneralChargeTimeForFireDelay", EditConditionHides, ClampMin = "0",
					  UIMin = "0", AllowPrivateAccess = true))
	float FireDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TInstancedStruct<FEffectData>> EffectDataInstances;
	float ChargeStartTime = 0.f;
	// Server-only. Keeps the add/remove of RemoteFireTag balanced when EndAbility runs more than once.
	bool bRemoteFireTagApplied = false;
};
