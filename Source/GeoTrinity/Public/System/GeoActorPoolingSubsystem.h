// GeoActorPoolSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GeoActorPoolingSubsystem.generated.h"

class AGeoProjectile;
class AActor;
class APawn;

UCLASS()
class GEOTRINITY_API UGeoActorPoolingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	/**
	 * Returns a pooled actor of the given class, or spawns a new one if the pool is empty.
	 *
	 * @param Class        The actor class to retrieve. Must derive from AActor.
	 * @param Transform    Spawn transform applied to the actor.
	 * @param Owner        The actor that logically owns this instance.
	 * @param Instigator   The pawn responsible for spawning (used for damage attribution).
	 * @param bInit        When true, calls IGeoPoolableInterface::Init() on the returned actor.
	 * @param bActivate    When true, calls actor.SetActorTickEnabled(true) and makes it visible.
	 * @return             The acquired actor cast to T, or nullptr if the cast fails.
	 */
	template <typename T>
	T* RequestActor(TSubclassOf<T> Class, FTransform const& Transform, AActor* Owner, APawn* Instigator,
					bool bInit = true, bool bActivate = true)
	{
		checkf((*Class)->IsChildOf(AActor::StaticClass()), TEXT("Class must be an AActor"));
		AActor* Spawned = PopWithClass(*Class, Transform, Owner, Instigator, bInit, bActivate);
		return Cast<T>(Spawned);
	}

	/**
	 * Pre-allocates Count inactive actors of the given class and stores them in the pool.
	 * Use during level load to avoid runtime spawn cost.
	 *
	 * @param Class        The actor class to pre-spawn. Must derive from AActor.
	 * @param Count        Number of instances to pre-allocate.
	 * @param Owner        Optional owner assigned to each pre-spawned actor.
	 * @param Instigator   Optional instigator assigned to each pre-spawned actor.
	 */
	template <typename T>
	void PreSpawn(TSubclassOf<T> Class, uint16 const Count, AActor* Owner = nullptr, APawn* Instigator = nullptr)
	{
		ensureMsgf(Class->IsChildOf(AActor::StaticClass()), TEXT("Class must be an AActor"));
		PreSpawn(*Class, Count, Owner, Instigator);
	}

	/** Returns Actor to the pool and calls IGeoPoolableInterface::End() on it. */
	void ReleaseActor(AActor* Actor);

	/** Returns the subsystem for the given world. */
	static UGeoActorPoolingSubsystem* Get(UWorld const* World);

	/** Returns the subsystem for the world of the given context object. Blueprint-callable. */
	UFUNCTION(BlueprintCallable, Category = "ActorPoolingSystem", meta = (WorldContext = "WorldContextObject"))
	static UGeoActorPoolingSubsystem* Get(UObject const* WorldContextObject);

	/**
	 * Returns true when Actor is currently active (i.e. in use by a caller, not sitting idle in the pool).
	 * An active actor has tick enabled and is visible; a pooled (inactive) actor has tick disabled and is hidden.
	 */
	static bool GetActorState(AActor const* Actor);

	/**
	 * Marks Actor as active or inactive and applies the corresponding visibility and tick state.
	 * Active (bActive=true): SetActorHiddenInGame(false) + SetActorTickEnabled(true).
	 * Inactive (bActive=false): SetActorHiddenInGame(true) + SetActorTickEnabled(false).
	 *
	 * @param NewActor  The actor whose state to change. Must not be null.
	 * @param bActive   True to activate (make visible and tickable), false to deactivate.
	 */
	static void ChangeActorState(AActor* NewActor, bool bActive);

private:
	// Non-templated helper for internal use
	AActor* PopWithClass(UClass* Class, FTransform const& Transform, AActor* Owner, APawn* Instigator, bool bInit,
						 bool bActivate);
	void PreSpawn(UClass* Class, uint16 const Count, AActor* Owner, APawn* Instigator);
	AActor* SpawnActor(UClass* Class, FActorSpawnParameters const& Params);
	// Storage for pooled (inactive) actors, keyed by concrete class
	TMap<TObjectPtr<UClass>, TArray<TWeakObjectPtr<AActor>>> Pool;
};
