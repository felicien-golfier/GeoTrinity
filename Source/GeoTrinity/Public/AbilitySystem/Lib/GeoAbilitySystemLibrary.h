// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
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
							  int32 AbilityLevel, int32 Seed);

	/** Applies a single TInstancedStruct<FEffectData> entry (calls UpdateContextHandle then ApplyEffect). */
	static FActiveGameplayEffectHandle ApplySingleEffectData(TInstancedStruct<FEffectData> const& Data,
															 UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															 int32 Seed);

	/** Applies a single FEffectData (calls UpdateContextHandle then ApplyEffect). */
	static FActiveGameplayEffectHandle ApplySingleEffectData(FEffectData const& EffectData,
															 UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															 int32 Seed);

	/**
	 * Returns the class default object for the ability registered under AbilityTag, cast to T.
	 * Asserts (ensureMsgf) if no matching ability is found or the CDO cannot be cast to T.
	 *
	 * @return  CDO pointer on success, nullptr if not found or wrong type.
	 */
	template <typename T>
	static T const* GetAbilityCDO(FGameplayTag const AbilityTag)
	{
		for (FGameplayAbilityInfo const& AbilityInfo : GetAbilityInfo()->GetAllAbilityInfos())
		{
			if (AbilityInfo.AbilityTag == AbilityTag)
			{
				UGameplayAbility const* AbilityCDO = AbilityInfo.AbilityClass.GetDefaultObject();
				if (IsValid(AbilityCDO) && AbilityCDO->IsA(T::StaticClass()))
				{
					return CastChecked<T>(AbilityCDO);
				}
			}
		}

		ensureMsgf(false, TEXT("No Ability found for AbilityTag %s"), *AbilityTag.ToString());
		return nullptr;
	}

	static UGeoGameplayAbility const* GetAbilityCDO(FGameplayTag AbilityTag);

	/** Extracts the EffectDataInstances array from a UEffectDataAsset. */
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(UEffectDataAsset const* EffectDataAsset);
	/** Returns the effect data array registered for the ability identified by AbilityTag in UAbilityInfo. */
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(FGameplayTag AbilityTag);

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
	 * Get target directions based on targeting mode.
	 * @param World The world context
	 * @param Target The targeting mode
	 * @param Yaw Direction for Forward mode
	 * @param Origin Position for calculating directions to players
	 * @return Array of direction vectors
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

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetAllAgentsInTeam(UObject const* WorldContextObject, FGenericTeamId const& TeamId);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetAllAgentsWithRelationTowardsActor(UObject const* WorldContextObject, AActor const* Actor,
																ETeamAttitude::Type Attitude);

	/** Converts a UE ETeamAttitude enum value to its corresponding ETeamAttitudeBitflag bit. */
	static ETeamAttitudeBitflag GetAttitudeBitflag(ETeamAttitude::Type Attitude);
	/** Returns true when AttitudeBitflag has the bit corresponding to Attitude set. */
	static bool IsAttitudeIntBitflag(ETeamAttitudeBitflag AttitudeBitflag, ETeamAttitude::Type Attitude);
	/** Returns true when OtherActor's team attitude toward Owner is set in OverlapAttitudeBitMask. */
	static bool IsTeamAttitudeAligned(AActor const* Owner, AActor const* OtherActor, int32 OverlapAttitudeBitMask);


	/** Returns the actor in ActorList with the smallest 3D distance to FromActor, or nullptr if the list is empty. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Toolbox")
	static AActor* GetNearestActorFromList(AActor const* FromActor, TArray<AActor*> const& ActorList);

	/**
	 * Getters and setters for the context
	 */
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool IsBlockedHit(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool IsCriticalHit(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool IsSuccessfulDebuff(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetDebuffDamage(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetDebuffDuration(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetDebuffFrequency(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FVector GetDeathImpulseVector(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FVector GetKnockbackVector(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static bool GetIsRadialDamage(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetRadialDamageInnerRadius(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static float GetRadialDamageOuterRadius(FGameplayEffectContextHandle const& effectContextHandle);
	UFUNCTION(BlueprintPure, Category = "AbilitySystemLibrary|GameplayEffects")
	static FVector GetRadialDamageOrigin(FGameplayEffectContextHandle const& effectContextHandle);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetIsBlockedHit(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
								bool const bIsBlockedHit);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetIsCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
								 bool const bIsCriticalHit);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetStatusTag(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
							 FGameplayTag const statusTag);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDebuffDamage(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
								float const debuffDamage);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDebuffFrequency(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
								   float const debuffFrequency);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDebuffDuration(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
								  float const debuffDuration);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetDeathImpulseVector(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
									  FVector const& inVector);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetKnockbackVector(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
								   FVector const& inVector);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetIsRadialDamage(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
								  bool const bIsRadialDamage);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageInnerRadius(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
										   float const radialDamageInnerRadius);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageOuterRadius(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
										   float const radialDamageOuterRadius);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|GameplayEffects")
	static void SetRadialDamageOrigin(UPARAM(ref) FGameplayEffectContextHandle& effectContextHandle,
									  FVector const& inVector);

	/** Returns the ASC from Actor cast to UGeoAbilitySystemComponent, or nullptr if Actor does not implement
	 * IAbilitySystemInterface. */
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary")
	static UGeoAbilitySystemComponent* GetGeoAscFromActor(AActor* Actor);

	/**
	 * END context getter setters
	 */
};

using GeoASLib = UGeoAbilitySystemLibrary;
