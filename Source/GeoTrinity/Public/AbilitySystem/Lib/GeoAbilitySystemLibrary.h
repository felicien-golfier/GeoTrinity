// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

#include "GeoAbilitySystemLibrary.generated.h"

UENUM(BlueprintType)
enum class EProjectileTarget : uint8
{
	Forward,
	AllPlayers
};

class AGeoDeployableBase;
class AGeoProjectile;
class UStatusInfo;
struct FDeployableData;
struct FDeployableDataParams;
struct FExternalProjectileParams;
struct FGeoGameplayEffectContext;
/**
 * Static helper library for GeoTrinity's ability system (alias: GeoASLib).
 * Centralizes two-pass effect application, projectile and deployable spawning, team-attitude
 * queries, gameplay cue utilities, and ability CDO lookups so individual ability classes can
 * focus on their own logic without reimplementing common GAS plumbing.
 */
UCLASS()
class GEOTRINITY_API UGeoAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr TCHAR const* SocketBaseName = TEXT("anim_socket_");
	inline static FName const SectionStartName{"Start"};
	inline static FString SectionStartString{SectionStartName.ToString()};
	inline static FName const SectionFireName{"Fire"};
	inline static FString SectionFireString{SectionFireName.ToString()};
	inline static FName const SectionEndName{"End"};
	inline static FString SectionEndString{SectionEndName.ToString()};
	inline static FName const SectionStopName{"Stop"};
	inline static FString SectionStopString{SectionStopName.ToString()};

	/**
	 * Returns the section index for Section in AnimMontage.
	 * @warning Asserts in debug if the section is not found — callers must use valid section names.
	 */
	static int GetAndCheckSection(UAnimMontage const* AnimMontage, FName Section);
	/** Returns the AnimInstance from the avatar actor stored in Payload.Owner. */
	static UAnimInstance* GetAnimInstance(FAbilityPayload const& Payload);

	/** Returns the global UAbilityInfo data asset from UGameDataSettings. */
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|Info")
	static UAbilityInfo* GetAbilityInfo();


	/**
	 * Applies all effect descriptors in DataArray using a two-pass strategy:
	 * first pass calls UpdateContextHandle on every entry, second pass calls ApplyEffect on every entry.
	 * This ensures context data (e.g. damage multipliers) is set before any effect is applied.
	 *
	 * @return  Array of active effect handles, one per successfully applied effect. Invalid handles are included for
	 * entries that do not apply effects.
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Effects")
	static TArray<FActiveGameplayEffectHandle>
	ApplyEffectFromEffectData(TArray<TInstancedStruct<FEffectData>> const& DataArray,
							  UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC,
							  int32 AbilityLevel, int32 Seed, FGameplayTag AbilityTag);

	/** Applies a single TInstancedStruct<FEffectData> entry (calls UpdateContextHandle then ApplyEffect). */
	static FActiveGameplayEffectHandle ApplySingleEffectData(TInstancedStruct<FEffectData> const& Data,
															 UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															 int32 Seed, FGameplayTag AbilityTag);

	/** Applies a single FEffectData (calls UpdateContextHandle then ApplyEffect). */
	static FActiveGameplayEffectHandle ApplySingleEffectData(FEffectData const& EffectData,
															 UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															 int32 Seed, FGameplayTag AbilityTag);
	/**
	 * Reports that Payload's shot connected with HitActor, broadcasting OnAbilityHit on the ASC behind Payload.Owner.
	 * Call it from wherever an ability or a projectile decides it hit — never from effect application, which ticks,
	 * defers and re-applies far away from the moment of impact.
	 *
	 * Only the first call per shot gets through: Payload.HitNotified is the handle a delayed carrier holds on the
	 * shot's behalf, and this consumes it. A payload carrying no handle reports nothing.
	 */
	static void NotifyAbilityHit(FAbilityPayload const& Payload, AActor* HitActor);

	/** Fills SourceASC and TargetASC into ContextHandle for access by downstream execution calculations. */
	static void FillEffectContext(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC,
								  FGameplayEffectContextHandle ContextHandle);

	/**
	 * Builds the effect context both apply paths run on: makes it from SourceASC, fills it, and hands back the Geo
	 * context to write scoped fields into. The returned handle owns the context — keep it alive for the whole
	 * application.
	 *
	 * @param OutGeoContext  Set to the context behind the returned handle; never null (checked).
	 */
	static FGameplayEffectContextHandle MakeGeoEffectContext(UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC,
															 FGeoGameplayEffectContext*& OutGeoContext);

	/**
	 * True when the GameplayCue embedded in the applied effect must not play: suppressed outright by the context, or
	 * spent from the target's UGeoGameFeelComponent budget for that kind of cue. The one rule both ExecCalcs ask.
	 *
	 * @param bIsHeal  Which budget the rate limit is taken from — the heal one, or the damage one.
	 */
	static bool ShouldSuppressGameplayCue(FGeoGameplayEffectContext const& GeoContext, AActor* TargetAvatar,
										  bool bIsHeal);

	/**
	 * Returns the class default object for the ability registered under AbilityTag, cast to T.
	 * O(1) via UAbilityInfo's cached tag->class map. Logs a warning and returns nullptr if no matching ability is found
	 * or the CDO cannot be cast to T. Callers that know a tag may legitimately be invalid should guard before calling
	 * (an invalid tag still logs the warning).
	 *
	 * @return  CDO pointer on success, nullptr if not found or wrong type.
	 */
	template <typename T>
	static T const* GetAbilityCDO(FGameplayTag const AbilityTag)
	{
		UAbilityInfo const* AbilityInfo = GetAbilityInfo();
		TSubclassOf<UGameplayAbility> const AbilityClass =
			AbilityInfo ? AbilityInfo->GetAbilityClassForTag(AbilityTag) : nullptr;
		UGameplayAbility const* AbilityCDO = AbilityClass ? AbilityClass.GetDefaultObject() : nullptr;
		if (IsValid(AbilityCDO) && AbilityCDO->IsA(T::StaticClass()))
		{
			return CastChecked<T>(AbilityCDO);
		}

		UE_LOG(LogTemp, Warning, TEXT("GetAbilityCDO: no ability found for AbilityTag %s"), *AbilityTag.ToString());
		return nullptr;
	}

	/** Non-template overload; returns the CDO cast to UGeoGameplayAbility const*. */
	static UGeoGameplayAbility const* GetAbilityCDO(FGameplayTag AbilityTag);

	/** Returns the first ability granted to ASC that is a T (or subclass), or nullptr when none is granted. */
	template <typename T>
	static T const* GetGrantedAbility(UAbilitySystemComponent const& ASC)
	{
		for (FGameplayAbilitySpec const& Spec : ASC.GetActivatableAbilities())
		{
			if (T const* Ability = Cast<T>(Spec.Ability))
			{
				return Ability;
			}
		}
		return nullptr;
	}

	/** Returns the effect data array registered for the ability identified by AbilityTag in UAbilityInfo. */
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(FGameplayTag AbilityTag);

	/**
	 * Fully spawns and initializes a deployable actor: deferred-spawn, fills FDeployableData, calls InitInteractable,
	 * then FinishSpawning.
	 *
	 * @param DeployableActorClass  The deployable class to spawn.
	 * @param Payload               Network sync data (owner, instigator, ability tag, seed, etc.).
	 * @param EffectDataArray       Effects the deployable applies on hit or expiry.
	 * @param Params                Designer config: size, life drain, blink duration, value.
	 * @param SpawnTransform        World transform to spawn at.
	 * @return                      The fully initialized deployable, or nullptr on failure.
	 */
	static AGeoDeployableBase* FullySpawnDeployable(TSubclassOf<AGeoDeployableBase> const DeployableActorClass,
													FAbilityPayload const& Payload,
													TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
													FDeployableDataParams const& Params,
													FTransform const& SpawnTransform);
	/** Deferred-spawns a deployable actor without calling FinishSpawning; caller must call InitInteractable and then
	 * FinishSpawning itself. Returns nullptr on failure. */
	static AGeoDeployableBase* StartSpawnDeployable(TSubclassOf<AGeoDeployableBase> DeployableActorClass, AActor* Owner,
													APawn* Instigator, FTransform const& SpawnTransform);
	/** Populates Data from Payload, EffectDataArray, and Params without calling InitInteractable. Used when the caller
	 * needs to modify Data fields before passing it to InitInteractable manually. */
	static void FillDeployableData(FDeployableData& Data, FAbilityPayload const& Payload,
								   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
								   FDeployableDataParams const& Params);

	/** PROJECTILES **/

	/**
	 * Fully spawns and activates a projectile from Params.ProjectileClass, applying Params
	 * (distance/speed/radius/colors). Calls StartSpawnProjectile then FinishSpawnProjectile (which fast-forwards
	 * position by elapsed ping time).
	 *
	 * @param World            The world to spawn into.
	 * @param Params           Projectile class plus its distance/speed/radius/color overrides.
	 * @param SpawnTransform   Initial world transform for the projectile.
	 * @param Payload          Network sync data (owner, instigator, origin, yaw, timing, seed).
	 * @param EffectDataArray  Effects to apply on hit.
	 * @param SpawnServerTime  Synchronized server time at spawn, used to fast-forward position by elapsed ping.
	 * @return                 The spawned projectile, or nullptr on failure.
	 */
	static AGeoProjectile* FullySpawnProjectile(UWorld* const World, FExternalProjectileParams const& Params,
												FTransform const& SpawnTransform, FAbilityPayload const& Payload,
												TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
												float SpawnServerTime, FPredictionKey PredictionKey = FPredictionKey{});

	/** Begins deferred spawn from Params.ProjectileClass, sets payload/effect data, and applies Params
	 * (distance/speed/radius/colors) before FinishSpawning. */
	static AGeoProjectile* StartSpawnProjectile(UWorld* World, FExternalProjectileParams const& Params,
												FTransform const& SpawnTransform, FAbilityPayload const& Payload,
												TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
												FPredictionKey PredictionKey = FPredictionKey{});

	/**
	 * Calls FinishSpawning on a deferred projectile and fast-forwards its position by the elapsed time
	 * since SpawnServerTime to align it with the client's predicted version.
	 */
	static void FinishSpawnProjectile(UWorld const* World, AGeoProjectile* Projectile, FTransform const& SpawnTransform,
									  float SpawnServerTime, FPredictionKey PredictionKey);
	/**
	 * Returns normalized launch directions for a projectile ability based on the targeting mode.
	 *
	 * @param World   The world context (used to enumerate players for AllPlayers mode).
	 * @param Target  Targeting mode: Forward fires along Yaw; AllPlayers fires one direction per live player.
	 * @param Yaw     Instigator facing yaw in degrees; defines the forward direction for Forward mode.
	 * @param Origin  3D world position of the fire socket; used to compute per-player directions.
	 * @return        Array of normalized direction vectors, one per intended projectile.
	 */
	static TArray<FVector> GetTargetDirections(UWorld const* World, EProjectileTarget Target, float Yaw,
											   FVector const& Origin);

	/**
	 * Fully spawns one projectile per direction returned by GetTargetDirections.
	 *
	 * @param Origin           World-space spawn point, shared by every projectile of the spread.
	 * @param Yaw              Instigator facing yaw in degrees, defining the Forward direction.
	 * @param SpawnServerTime  Synchronized server time at spawn, used to fast-forward position by elapsed ping.
	 * @return                 Number of projectiles actually spawned.
	 */
	static int32 SpawnProjectileSpread(UWorld* World, FExternalProjectileParams const& Params, EProjectileTarget Target,
									   FVector const& Origin, float Yaw, float SpawnServerTime,
									   FAbilityPayload const& Payload,
									   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
									   FPredictionKey PredictionKey = FPredictionKey{});

	/** Returns Ability's first asset tag sitting under Root, or an invalid tag when it carries none. The single
	 * "which tag identifies this ability" walk — ability tag, ability type, and any future root all go through it. */
	static FGameplayTag GetFirstAssetTagUnderRoot(UGameplayAbility const& Ability, FGameplayTag Root);
	/** Returns the first asset tag under the "Ability" root from the spec's ability CDO. */
	static FGameplayTag GetAbilityTagFromSpec(FGameplayAbilitySpec const& Spec);
	/** Returns the first asset tag under the "Ability" root from the given ability CDO. */
	static FGameplayTag GetAbilityTagFromAbility(UGameplayAbility const& Ability);

	/** Returns the GenericTeamId for Actor, or FGenericTeamId::NoTeam if Actor does not implement
	 * IGenericTeamAgentInterface. */
	static FGenericTeamId GetTeamId(AActor const* Actor);

	/**
	 * Type-filtered variant of GetInteractableActors: returns only agents that are T (or a subclass of T),
	 * cast to T*. Wraps the ExtraFilter overload; avoids explicit casting at call sites.
	 */
	template <typename T>
	static TArray<T*> GetInteractableActors(UObject const* WorldContextObject, FGenericTeamId const SourceTeam,
											int32 AttitudeBitmask, bool bMustBeDamageable, FVector2D Location,
											float MaxDistance,
											ETargetOverlapMode OverlapMode = ETargetOverlapMode::Automatic)
	{
		TArray<T*> Result;
		for (AActor* Actor : GetInteractableActors(
				 WorldContextObject, SourceTeam, AttitudeBitmask, bMustBeDamageable, Location, MaxDistance,
				 [](AActor* Actor)
				 {
					 return IsValid(Actor) && Actor->IsA(T::StaticClass());
				 },
				 OverlapMode))
		{
			Result.Add(CastChecked<T>(Actor));
		}
		return Result;
	}

	/**
	 * Returns all interactable agents whose attitude toward SourceTeam matches any bit in AttitudeBitmask.
	 * @param AttitudeBitmask    Bitmask of ETeamAttitudeBitflag values (e.g. Hostile | Neutral).
	 * @param bMustBeDamageable  If true, skips actors that cannot be damaged.
	 * @param Location           2D world origin for the distance check.
	 * @param MaxDistance        Maximum distance in world units. 0 = no distance check.
	 * @param ExtraFilter        Optional per-actor predicate; actors for which it returns false are excluded.
	 * @param OverlapMode        How a target's own collision radius counts toward the distance test.
	 */
	static TArray<AActor*> GetInteractableActors(UObject const* WorldContextObject, FGenericTeamId const SourceTeam,
												 int32 AttitudeBitmask, bool bMustBeDamageable, FVector2D Location,
												 float MaxDistance, TFunctionRef<bool(AActor*)> const& ExtraFilter,
												 ETargetOverlapMode OverlapMode = ETargetOverlapMode::Automatic);

	/** Same as above without an extra filter. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetInteractableActors(UObject const* WorldContextObject, FGenericTeamId const SourceTeam,
												 int32 AttitudeBitmask, bool bMustBeDamageable = false,
												 FVector2D Location = FVector2D::ZeroVector, float MaxDistance = 0.f,
												 ETargetOverlapMode OverlapMode = ETargetOverlapMode::Automatic);

	/**
	 * Returns all interactable agents that lie within a beam segment.
	 * An actor passes if its collision circle (SimpleCollisionRadius) overlaps the segment
	 * [Origin, Origin + ForwardVector * MaxRange].
	 * @param Origin         2D start of the beam.
	 * @param ForwardVector  Normalized 2D beam direction.
	 * @param MaxRange       Beam length in world units.
	 * @param LineHalfWidth  Half-width added to each target's SimpleCollisionRadius for hit testing. 0 = point test.
	 * @param OverlapMode    How a target's own collision radius counts toward the hit test (the beam half-width is
	 *                       always kept; CenterOnly only drops the target's body radius).
	 */
	static TArray<AActor*> GetInteractableActorsInLine(UObject const* WorldContextObject, FGenericTeamId SourceTeam,
													   int32 AttitudeBitmask, bool bMustBeDamageable, FVector2D Origin,
													   FVector2D ForwardVector, float MaxRange,
													   float LineHalfWidth = 0.f,
													   ETargetOverlapMode OverlapMode = ETargetOverlapMode::Automatic);

	/** Converts a UE ETeamAttitude enum value to its corresponding ETeamAttitudeBitflag bit. */
	static ETeamAttitudeBitflag GetAttitudeBitflag(ETeamAttitude::Type Attitude);
	/** Returns true when AttitudeBitflag has the bit corresponding to Attitude set. */
	static bool IsAttitudeIntBitflag(ETeamAttitudeBitflag AttitudeBitflag, ETeamAttitude::Type Attitude);
	/** Returns true when OtherActor's team attitude toward Owner is set in OverlapAttitudeBitMask. */
	static bool IsTeamAttitudeAligned(AActor const* Owner, AActor const* OtherActor, uint8 OverlapAttitudeBitMask);

	/** Resolves OverlapMode for a query cast by SourceTeam: whether targets' own collision radius is counted. */
	static bool ShouldIncludeTargetRadius(ETargetOverlapMode OverlapMode, FGenericTeamId SourceTeam);


	/** Returns the actor in ActorList with the smallest 3D distance to FromActor, or nullptr if the list is empty. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Toolbox")
	static AActor* GetNearestActorFromList(AActor const* FromActor, TArray<AActor*> const& ActorList);

	/**
	 * Executes CueTag on ASC locally, on this machine only, via InvokeGameplayCueEvent(Executed).
	 * Use for cosmetics fired by logic that already runs on every relevant machine (OnReps, locally-gated ability
	 * code): ExecuteGameplayCue would additionally multicast from a listen-server host, double-playing the cue on
	 * clients. No-op when CueTag is invalid (optional cue not configured).
	 */
	static void ExecuteLocalGameplayCue(UAbilitySystemComponent* ASC, FGameplayTag const& CueTag,
										FGameplayCueParameters const& CueParams);

	/** Returns the status gameplay tag stored in the effect context (invalid tag when none). */
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FGameplayTag GetStatusTag(FGameplayEffectContextHandle const& EffectContextHandle);
	/** Sets the status gameplay tag in the effect context. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetStatusTag(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, FGameplayTag StatusTag);

	/** Returns the ASC from Actor cast to UGeoAbilitySystemComponent, or nullptr if Actor does not implement
	 * IAbilitySystemInterface. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary")
	static UGeoAbilitySystemComponent* GetGeoAscFromActor(AActor* Actor);
};

using GeoASLib = UGeoAbilitySystemLibrary;
