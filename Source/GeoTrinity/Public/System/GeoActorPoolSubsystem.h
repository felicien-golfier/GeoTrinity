// GeoActorPoolSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GeoActorPoolSubsystem.generated.h"

class AGeoProjectile;

UCLASS()
class GEOTRINITY_API UGeoActorPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    // Generic C++ templated acquire for any AActor subclass
    template<typename T>
    T* SpawnPooled(TSubclassOf<T> Class, const FTransform& Transform, AActor* Owner = nullptr, APawn* Instigator = nullptr)
    {
        AActor* Spawned = SpawnPooledClass(*Class, Transform, Owner, Instigator);
        return Cast<T>(Spawned);
    }

    // Blueprint-accessible acquire for projectiles (works with any subclass of AGeoProjectile)
    UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject"))
    AGeoProjectile* SpawnPooledProjectile(const UObject* WorldContextObject, TSubclassOf<AGeoProjectile> Class, const FTransform& Transform);

    // Return any actor to its pool
    UFUNCTION(BlueprintCallable, Category = "Pooling")
    void ReleaseActor(AActor* Actor);

    // Non-templated helper for internal use
    AActor* SpawnPooledClass(UClass* Class, const FTransform& Transform, AActor* Owner, APawn* Instigator);

private:
    // Storage for pooled (inactive) actors, keyed by concrete class
    TMap<TObjectPtr<UClass>, TArray<TWeakObjectPtr<AActor>>> InactiveByClass;
};
