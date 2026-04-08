// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

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

	static int GetAndCheckSection(UAnimMontage const* AnimMontage, FName Section);
	static UAnimInstance* GetAnimInstance(FAbilityPayload const& Payload);

	/** INFO HOLDER **/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Info", meta = (DefaultToSelf = "WorldContextObject"))
	static UAbilityInfo* GetAbilityInfo(UObject const* WorldContextObject);
	static UAbilityInfo* GetAbilityInfo();
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Info", meta = (DefaultToSelf = "WorldContextObject"))
	static UStatusInfo* GetStatusInfo(UObject const* WorldContextObject);


	/** PARAMS STUFF **/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Effects")
	static TArray<FActiveGameplayEffectHandle>
	ApplyEffectFromEffectData(TArray<TInstancedStruct<FEffectData>> const& DataArray,
							  UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC,
							  int32 AbilityLevel, int32 Seed);

	static FActiveGameplayEffectHandle ApplySingleEffectData(TInstancedStruct<FEffectData> const& Data,
															 UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															 int32 Seed);

	static FActiveGameplayEffectHandle ApplySingleEffectData(FEffectData const& EffectData,
															 UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															 int32 Seed);

	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(UEffectDataAsset const* EffectDataAsset);
	static TArray<TInstancedStruct<FEffectData>> GetEffectDataArray(FGameplayTag AbilityTag);

	/** PROJECTILES **/

	/**
	 * Spawns a projectile from the actor pool with the given payload and effects.
	 * @param World The world context
	 * @param ProjectileClass The projectile class to spawn
	 * @param SpawnTransform Where to spawn the projectile
	 * @param Payload Network sync data (owner, instigator, origin, yaw, timing, seed)
	 * @param EffectDataArray Effects to apply on hit
	 * @param SpawnServerTime
	 * @return The spawned projectile, or nullptr on failure
	 */
	static AGeoProjectile* SpawnProjectile(UWorld* const World, TSubclassOf<AGeoProjectile> ProjectileClass,
										   FTransform const& SpawnTransform, FAbilityPayload const& Payload,
										   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
										   float SpawnServerTime, FPredictionKey PredictionKey = FPredictionKey{});

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

	/** STATUS **/
	static bool ApplyStatusToTarget(UAbilitySystemComponent* pTargetASC, UAbilitySystemComponent* pSourceASC,
									FGameplayTag const& statusTag, int32 level,
									FGameplayEffectSpecHandle& OutSpecHandle);

	/** TAG **/
	static FGameplayTag GetGameplayTagFromRootTagString(FString const& StringOfTag);
	static FGameplayTag GetAbilityTagFromSpec(FGameplayAbilitySpec const& Spec);
	static FGameplayTag GetAbilityTagFromAbility(UGameplayAbility const& Ability);

	/** TEAM **/
	static bool GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetAllAgentsInTeam(UObject const* WorldContextObject, FGenericTeamId const& TeamId);
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary|Team", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AActor*> GetAllAgentsWithRelationTowardsActor(UObject const* WorldContextObject, AActor const* Actor,
																ETeamAttitude::Type Attitude);

	static ETeamAttitudeBitflag GetAttitudeBitflag(ETeamAttitude::Type Attitude);
	static bool IsAttitudeIntBitflag(ETeamAttitudeBitflag AttitudeBitflag, ETeamAttitude::Type Attitude);
	static bool IsTeamAttitudeAligned(AActor const* Owner, AActor const* OtherActor, int32 OverlapAttitudeBitMask);


	/** TOOLBOX **/
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

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemLibrary")
	static UGeoAbilitySystemComponent* GetGeoAscFromActor(AActor* Actor);

	/**
	 * END context getter setters
	 */
};

using GeoASLib = UGeoAbilitySystemLibrary;
