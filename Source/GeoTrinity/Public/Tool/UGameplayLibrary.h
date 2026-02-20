#pragma once
#include "GameplayPrediction.h"
#include "GenericTeamAgentInterface.h"
#include "StructUtils/InstancedStruct.h"

#include "UGameplayLibrary.generated.h"


UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETeam : uint8
{
	Neutral = (1 << 0) UMETA(DisplayName = "Neutral"),
	Ally = (1 << 1) UMETA(DisplayName = "Ally"),
	Enemy = (1 << 2) UMETA(DisplayName = "Enemy")
};

UENUM(BlueprintType)
enum class EProjectileTarget : uint8
{
	Forward,
	AllPlayers
};

class AGeoProjectile;
struct FAbilityPayload;
struct FEffectData;

UCLASS()

class UGameplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface);

	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static FColor GetColorForObject(UObject const* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static bool IsServer(UObject const* WorldContextObject);
	static bool IsServer(UWorld const* World);

	UFUNCTION(BlueprintCallable, Category = "GameplayLibrary", meta = (DefaultToSelf = "WorldContextObject"))
	static float GetServerTime(UObject const* WorldContextObject, bool bUpdatedWithPing = false);
	static float GetServerTime(UWorld const* World, bool bUpdatedWithPing = false);

	static int GetAndCheckSection(UAnimMontage const* AnimMontage, FName Section);
	static UAnimInstance* GetAnimInstance(FAbilityPayload const& Payload);

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

	inline static FName const SectionStartName{"Start"};
	inline static FString SectionStartString{SectionStartName.ToString()};
	inline static FName const SectionFireName{"Fire"};
	inline static FString SectionFireString{SectionFireName.ToString()};
	inline static FName const SectionEndName{"End"};
	inline static FString SectionEndString{SectionEndName.ToString()};
	inline static FName const SectionStopName{"Stop"};
	inline static FString SectionStopString{SectionStopName.ToString()};
};
