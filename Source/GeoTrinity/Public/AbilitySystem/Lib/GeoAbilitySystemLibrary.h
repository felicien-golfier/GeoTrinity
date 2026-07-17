// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Actor/Deployable/GeoDeployableBase.h"
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

class UStatusInfo;
struct FDamageEffectParams;
class UAbilityInfo;
struct FGameplayAbilitySpec;
class UGameplayAbility;
struct FGameplayEffectContextHandle;
struct FGameplayTag;
class UAbilitySystemComponent;
/**
 * A library of helper functions for the ASC
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
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Info", meta = (DefaultToSelf = "WorldContextObject"))
	static UAbilityInfo* GetAbilityInfo(UObject const* WorldContextObject);
	/** Returns the global UAbilityInfo data asset from UGameDataSettings without a world context. */
	static UAbilityInfo* GetAbilityInfo();
	/** Returns the global UStatusInfo data asset from UGameDataSettings. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Info", meta = (DefaultToSelf = "WorldContextObject"))
	static UStatusInfo* GetStatusInfo(UObject const* WorldContextObject);


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
	/** Fills SourceASC and TargetASC into ContextHandle for access by downstream execution calculations. */
	static void FillEffectContext(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC,
								  FGameplayEffectContextHandle ContextHandle);

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

	/** Extracts the EffectDataInstances array from a UEffectDataAsset. */
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(UEffectDataAsset const* EffectDataAsset);
	/** Returns the effect data array registered for the ability identified by AbilityTag in UAbilityInfo. */
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(FGameplayTag AbilityTag);

	/**
	 * Fully spawns and initializes a deployable actor: deferred-spawn, fills FDeployableData, calls InitInteractable,
	 * then FinishSpawning. Equivalent to calling StartSpawnDeployable → InitDeployable → FinishSpawnDeployable in one
	 * call.
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
	/** Deferred-spawns a deployable actor without calling FinishSpawning; caller must call FinishSpawnDeployable
	 * after configuring the deployable's data (via InitInteractable). Returns nullptr on failure. */
	static AGeoDeployableBase* StartSpawnDeployable(TSubclassOf<AGeoDeployableBase> DeployableActorClass, AActor* Owner,
													APawn* Instigator, FTransform const& SpawnTransform);
	/** Fills FDeployableData from Payload + Params + EffectDataArray and calls InitInteractable on Deployable. */
	static void InitDeployable(AGeoDeployableBase* Deployable, FAbilityPayload const& Payload,
							   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
							   FDeployableDataParams const& Params);
	/** Populates Data from Payload, EffectDataArray, and Params without calling InitInteractable. Used when the caller
	 * needs to modify Data fields before passing it to InitInteractable manually. */
	static void FillDeployableData(FDeployableData& Data, FAbilityPayload const& Payload,
								   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
								   FDeployableDataParams const& Params);
	/** Completes a deferred deployable spawn started by StartSpawnDeployable; triggers BeginPlay. */
	static void FinishSpawnDeployable(AGeoDeployableBase* Deployable, FTransform const& SpawnTransform);

	/** PROJECTILES **/

	/**
	 * Fully spawns and activates a projectile from the actor pool.
	 * Calls StartSpawnProjectile followed by FinishSpawnProjectile (which fast-forwards position by elapsed ping time).
	 *
	 * @param World            The world to spawn into.
	 * @param ProjectileClass  The projectile class to spawn.
	 * @param SpawnTransform   Initial world transform for the projectile.
	 * @param Payload          Network sync data (owner, instigator, origin, yaw, timing, seed).
	 * @param EffectDataArray  Effects to apply on hit.
	 * @param SpawnServerTime  Synchronized server time at spawn, used to fast-forward position by elapsed ping.
	 * @return                 The spawned projectile, or nullptr on failure.
	 */
	static AGeoProjectile* FullySpawnProjectile(UWorld* const World, TSubclassOf<AGeoProjectile> ProjectileClass,
												FTransform const& SpawnTransform, FAbilityPayload const& Payload,
												TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
												float SpawnServerTime, FPredictionKey PredictionKey = FPredictionKey{});

	/** Begins deferred spawn of a projectile, sets its payload and effect data, but does not call FinishSpawning. */
	static AGeoProjectile* StartSpawnProjectile(UWorld* World, TSubclassOf<AGeoProjectile> ProjectileClass,
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
	 * Looks up the status GE for statusTag in UStatusInfo and applies it to pTargetASC.
	 *
	 * @param OutSpecHandle  Populated with the applied spec handle on success.
	 * @return               True if the status GE was found and applied.
	 */
	static bool ApplyStatusToTarget(UAbilitySystemComponent* pTargetASC, UAbilitySystemComponent* pSourceASC,
									FGameplayTag const& statusTag, int32 level,
									FGameplayEffectSpecHandle& OutSpecHandle);

	/** Returns the gameplay tag that has StringOfTag as its leaf name, searching within the global tag container. */
	static FGameplayTag GetGameplayTagFromRootTagString(FString const& StringOfTag);
	/** Returns the first asset tag under the "Ability" root from the spec's ability CDO. */
	static FGameplayTag GetAbilityTagFromSpec(FGameplayAbilitySpec const& Spec);
	/** Returns the first asset tag under the "Ability" root from the given ability CDO. */
	static FGameplayTag GetAbilityTagFromAbility(UGameplayAbility const& Ability);

	/**
	 * Retrieves the IGenericTeamAgentInterface from Actor.
	 *
	 * @param OutInterface  Set to the found interface on success.
	 * @return              True if Actor implements the interface.
	 */
	static bool GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface);

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
											float MaxDistance)
	{
		TArray<T*> Result;
		for (AActor* Actor : GetInteractableActors(WorldContextObject, SourceTeam, AttitudeBitmask, bMustBeDamageable,
												   Location, MaxDistance,
												   [](AActor* Actor)
												   {
													   return IsValid(Actor) && Actor->IsA(T::StaticClass());
												   }))
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
	 */
	static TArray<AActor*> GetInteractableActors(UObject const* WorldContextObject, FGenericTeamId const SourceTeam,
												 int32 AttitudeBitmask, bool bMustBeDamageable, FVector2D Location,
												 float MaxDistance, TFunctionRef<bool(AActor*)> const& ExtraFilter);

	/** Same as above without an extra filter. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetInteractableActors(UObject const* WorldContextObject, FGenericTeamId const SourceTeam,
												 int32 AttitudeBitmask, bool bMustBeDamageable, FVector2D Location,
												 float MaxDistance);

	/** Returns all interactable agents matching AttitudeBitmask relative to SourceTeam, without a distance filter. */
	static TArray<AActor*> GetInteractableActors(UObject const* WorldContextObject, FGenericTeamId const SourceTeam,
												 int32 AttitudeBitmask, bool bMustBeDamageable = true);
	/** Returns all interactable agents within MaxDistance of Location regardless of team. */
	static TArray<AActor*> GetInteractableActors(UObject const* WorldContextObject, bool bMustBeDamageable,
												 FVector2D Location, float MaxDistance);

	/**
	 * Returns all interactable agents that lie within a beam segment.
	 * An actor passes if its collision circle (SimpleCollisionRadius) overlaps the segment
	 * [Origin, Origin + ForwardVector * MaxRange].
	 * @param Origin         2D start of the beam.
	 * @param ForwardVector  Normalized 2D beam direction.
	 * @param MaxRange       Beam length in world units.
	 * @param LineHalfWidth  Half-width added to each target's SimpleCollisionRadius for hit testing. 0 = point test.
	 */
	static TArray<AActor*> GetInteractableActorsInLine(UObject const* WorldContextObject, FGenericTeamId SourceTeam,
													   int32 AttitudeBitmask, bool bMustBeDamageable, FVector2D Origin,
													   FVector2D ForwardVector, float MaxRange,
													   float LineHalfWidth = 0.f);

	/** Converts a UE ETeamAttitude enum value to its corresponding ETeamAttitudeBitflag bit. */
	static ETeamAttitudeBitflag GetAttitudeBitflag(ETeamAttitude::Type Attitude);
	/** Returns true when AttitudeBitflag has the bit corresponding to Attitude set. */
	static bool IsAttitudeIntBitflag(ETeamAttitudeBitflag AttitudeBitflag, ETeamAttitude::Type Attitude);
	/** Returns true when OtherActor's team attitude toward Owner is set in OverlapAttitudeBitMask. */
	static bool IsTeamAttitudeAligned(AActor const* Owner, AActor const* OtherActor, uint8 OverlapAttitudeBitMask);


	/** Returns the actor in ActorList with the smallest 3D distance to FromActor, or nullptr if the list is empty. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Toolbox")
	static AActor* GetNearestActorFromList(AActor const* FromActor, TArray<AActor*> const& ActorList);

	/** Returns the status gameplay tag stored in the effect context (invalid tag when none). */
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FGameplayTag GetStatusTag(FGameplayEffectContextHandle const& effectContextHandle);
	/** Sets the status gameplay tag in the effect context. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetStatusTag(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
							 FGameplayTag const statusTag);

	/** Returns the ASC from Actor cast to UGeoAbilitySystemComponent, or nullptr if Actor does not implement
	 * IAbilitySystemInterface. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary")
	static UGeoAbilitySystemComponent* GetGeoAscFromActor(AActor* Actor);
};

using GeoASLib = UGeoAbilitySystemLibrary;
