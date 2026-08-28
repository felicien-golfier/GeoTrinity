// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystemComponent.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"

#include "GeoAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealProvided, float, HealDone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageDealt, float, DamageAmount, FGameplayTag, AbilityTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAbilityHit, FGameplayTag, AbilityTag, AActor*, Instigator, AActor*,
											   HitActor);

class UGeoGameplayAbility;
struct FGeoGameplayEffectContext;

/** One watched ability: the CDO that knows how to replay it, and the timer driving its shots. */
USTRUCT()
struct FRemoteFire
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UGeoGameplayAbility> AbilityCDO;

	FTimerHandle ShotTimer;
};

/**
 * Ability system component tailored for the GeoTrinity 2D top-down game.
 * Adds input-tag-driven ability activation, startup ability management per player class,
 * attribute delegate binding, and the pattern replication system for enemy bullet patterns.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Pre-creates pattern instances for every UPatternAbility-derived ability in StartupAbilityTags so PatternStartMulticast can always find one; no-op in non-game worlds. */
	virtual void InitializeComponent() override;
	/** Binds health and speed attribute change delegates; also registers ally remote-fire tag listeners on clients so cosmetic shot replay works for observed allies. */
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	/** Grants every ability in StartupAbilityTags, looked up from the global UAbilityInfo asset. */
	void GiveStartupAbilities();

	/** Grants all abilities defined for PlayerClass in the global UAbilityInfo asset. */
	void GiveStartupAbilities(EPlayerClass PlayerClass);

	/** Removes all abilities that were granted for a specific player class. */
	void ClearPlayerClassAbilities();

	/** Notifies the ASC that the input mapped to InputTag was pressed this frame. */
	void AbilityInputTagPressed(FGameplayTag const& InputTag);

	/** Notifies the ASC that the input mapped to InputTag is held this frame. */
	void AbilityInputTagHeld(FGameplayTag const& InputTag);

	/** Notifies the ASC that the input mapped to InputTag was released this frame. */
	void AbilityInputTagReleased(FGameplayTag const& InputTag);

	/** Activates ability with event data containing avatar orientation. Used for projectile abilities. */
	bool TryActivateAbilityWithTargetData(FGameplayAbilitySpecHandle Handle, FGameplayTag AbilityTag);

	/**
	 * Re-activates every granted passive ability. Passives auto-activate once on grant (OnAvatarSet); after death
	 * cancels them they must be restarted explicitly, since the avatar is not re-set on revive.
	 */
	void ReactivatePassiveAbilities();

	/**
	 * Clears every ability cooldown: the server drops the cooldown effects, and each machine drops the cooldown tags it
	 * still holds locally.
	 *
	 * Must be called from a path that runs on every machine — drive it from replicated state (bIsDead, OnRep_PlayerClass),
	 * never from the server alone. A client raises the cooldown tag itself when it predicts an activation and never
	 * lowers it; only the replicated count does, and it cannot when the server drops the effect on the frame it was
	 * applied — the count then goes 0 -> 1 -> 0 between two replication passes and no delta is ever sent.
	 */
	void ResetCooldowns();

	/** Applies a gameplay effect to this component's owner at CombatLevel. */
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass);

	/** Applies the DefaultAttributes gameplay effect to initialize base attribute values. */
	void InitializeDefaultAttributes();

	/**
	 * Sets the level everything this component grants or applies is stamped with. A boss's is the EGeoDifficulty bit
	 * of its arena, so one write levels its whole kit. Call it before GiveStartupAbilities and
	 * InitializeDefaultAttributes — neither re-levels what has already been granted or applied.
	 */
	void SetCombatLevel(int32 Level) { CombatLevel = Level; }
	/** Level every ability spec, effect and payload from this component carries. */
	int32 GetCombatLevel() const { return CombatLevel; }

	/** Binds OnHealthChanged and OnMaxHealthChanged delegates to the Health/MaxHealth attribute change callbacks. */
	void BindAttributeCallbacks();

	/**
	 * Instantiates a UPattern of PatternClass, registers it, and calls OnCreate.
	 *
	 * @param PatternClass  The UClass of the pattern to create. Must not be null.
	 * @param AbilityTag    Tag of the ability that spawned the pattern (used for montage selection).
	 * @return              The newly created pattern instance.
	 */
	UPattern* CreatePatternInstance(UClass const* PatternClass, FGameplayTag AbilityTag);

	/**
	 * Finds an active pattern instance by class.
	 *
	 * @param PatternClass  The class to search for.
	 * @param Pattern       Output — set to the found pattern, or null if not found.
	 * @return              True if a matching pattern was found.
	 */
	bool FindPatternByClass(UClass* PatternClass, UPattern*& Pattern);

	/** Ends all currently active pattern instances managed by this component. */
	void StopAllActivePatterns();

	/**
	 * Multicast RPC that instantiates PatternClass on every client and calls InitPattern with Payload.
	 * PatternData carries optional pattern-specific replicated data (an FPatternData subclass); unset for patterns that
	 * need none. Called by UPatternAbility on the server after activation.
	 */
	UFUNCTION(NetMulticast, reliable)
	void PatternStartMulticast(FAbilityPayload Payload, UClass* PatternClass,
							   TInstancedStruct<FPatternData> PatternData);

	/**
	 * Returns a reference to the per-ability fire section index, used to cycle animation montage sections.
	 * The index is created at zero on first access.
	 *
	 * @param AbilityTag  Tag identifying the ability whose fire section index to retrieve.
	 */
	int32& GetFireSectionIndex(FGameplayTag const& AbilityTag);

	/** Records the actor most recently hit by this owner's basic ability. Server-only; set from ExecCalc_Damage. */
	void SetLastBasicAbilityTarget(AActor* Target) { LastBasicAbilityTarget = Target; }
	/** Returns the actor most recently hit by this owner's basic ability, or nullptr if it died or never existed. */
	AActor* GetLastBasicAbilityTarget() const { return LastBasicAbilityTarget.Get(); }

	UPROPERTY(BlueprintAssignable)
	FOnHealProvided OnHealProvided;

	UPROPERTY(BlueprintAssignable)
	FOnDamageDealt OnDamageDealt;

	/**
	 * One broadcast per shot, the first time that shot connects with anything — one for a whole projectile spread, one
	 * per shot of a held trigger, one for a beam channel. Raised by GeoASLib::NotifyAbilityHit from the ability's or
	 * the projectile's own hit path, never from effect application, which ticks, defers and re-applies far from the
	 * moment of impact. Fires on the server, and on the machine that predicted the shot.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnAbilityHit OnAbilityHit;

	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;

private:
	/** Grants AbilityClass at CombatLevel, tagging the spec with InputTag when it is valid. The single grant path
	 * shared by both GiveStartupAbilities overloads. */
	void GiveAbilitySpec(TSubclassOf<UGameplayAbility> AbilityClass, FGameplayTag InputTag);

	/**
	 * Walks the granted specs matching InputTag and pushes the press into them. bFreshPress selects the polarity:
	 * true is the frame the key went down (activates bActivateOnFreshPressOnly abilities, handles the alternate
	 * release, replicates InputPressed), false is the per-frame hold (activates everything else).
	 */
	void ActivateAbilitiesForInput(FGameplayTag const& InputTag, bool bFreshPress);

	/** Returns the prediction key an already-activated spec is running under — the live instance's when there is one.
	 * Wraps the deprecated-API workaround so an engine upgrade has one place to fix. */
	static FPredictionKey GetActivationPredictionKey(FGameplayAbilitySpec const& AbilitySpec);

	/** Pipes an input release into an active spec: notifies its instances and replicates the InputReleased event. */
	void ReleaseAbilitySpec(FGameplayAbilitySpec& AbilitySpec);

	/**
	 * Client-side. Registers a tag-change listener for every ability CDO that declares a RemoteFireTag: an ally's
	 * abilities never instance on this machine, so their shots are replayed from the CDO instead. No-op once bound.
	 */
	void BindRemoteFireTags();
	/** Starts (tag added) or stops (tag removed) the replay timer for the ability owning RemoteFireTag. */
	void OnRemoteFireTagChanged(FGameplayTag RemoteFireTag, int32 NewCount);
	/** Timer callback: replays one shot of the ability owning RemoteFireTag through its CDO. */
	void RemoteFireShot(FGameplayTag RemoteFireTag);

	// Keyed by RemoteFireTag. Doubles as the bound/unbound flag for BindRemoteFireTags.
	UPROPERTY(Transient)
	TMap<FGameplayTag, FRemoteFire> RemoteFires;

	TMap<FGameplayTag, int32> FireSectionIndices;
	// Ability level of everything this component grants or applies. Stays 1 for players, who have no difficulty.
	int32 CombatLevel{1};
	bool bStartupAbilitiesGiven{false};
	bool bAttributeCallbacksBound{false};

	// Server-only: weak so a destroyed target reads back null without manual cleanup.
	TWeakObjectPtr<AActor> LastBasicAbilityTarget;

	UPROPERTY(Transient)
	TArray<UPattern*> Patterns;

	// DATA //
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	UPROPERTY(EditAnywhere, Category = "GAS")
	TArray<FGameplayTag> StartupAbilityTags;
};
