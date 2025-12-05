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
	// Generic C++ templated acquire for any AActor subclass
	template<typename T>
	T* Pop(TSubclassOf<T> Class, const FTransform& Transform, AActor* Owner, APawn* Instigator, bool bInit = true)
	{
		checkf((*Class)->IsChildOf(AActor::StaticClass()), TEXT("Class must be an AActor"));
		AActor* Spawned = PopWithClass(*Class, Transform, Owner, Instigator, bInit);
		return Cast<T>(Spawned);
	}

	// Pre-spawn a number of actors of the given class and store them inactive in the pool
	template<typename T>
	void PreSpawn(TSubclassOf<T> Class, const uint16 Count, AActor* Owner = nullptr, APawn* Instigator = nullptr)
	{
		ensureMsgf(Class->IsChildOf(AActor::StaticClass()), TEXT("Class must be an AActor"));
		PreSpawn(*Class, Count, Owner, Instigator);
	}

	// Return any actor to its pool
	void Push(AActor* Actor);

	static UGeoActorPoolingSubsystem* Get(const UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "ActorPoolingSystem", meta = (WorldContext = "WorldContextObject"))
	static UGeoActorPoolingSubsystem* Get(const UObject* WorldContextObject);

private:
	// Non-templated helper for internal use
	AActor* PopWithClass(UClass* Class, const FTransform& Transform, AActor* Owner, APawn* Instigator, bool bInit);
	void PreSpawn(UClass* Class, const uint16 Count, AActor* Owner, APawn* Instigator);
	void ChangeActorState(AActor* NewActor, bool bActive);
	AActor* SpawnActor(UClass* Class, const FActorSpawnParameters& Params);
	// Storage for pooled (inactive) actors, keyed by concrete class
	TMap<TObjectPtr<UClass>, TArray<TWeakObjectPtr<AActor>>> Pool;
};
