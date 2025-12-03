// GeoActorPoolSubsystem.cpp

#include "System/GeoActorPoolSubsystem.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"

AActor* UGeoActorPoolSubsystem::SpawnPooledClass(UClass* Class, const FTransform& Transform, AActor* Owner, APawn* Instigator)
{
    if (!ensure(Class))
    {
        return nullptr;
    }

    // Try reuse from pool
    TArray<TWeakObjectPtr<AActor>>& Pool = InactiveByClass.FindOrAdd(Class);
    while (Pool.Num() > 0)
    {
        TWeakObjectPtr<AActor> Weak = Pool.Pop();
        if (AActor* Actor = Weak.Get())
        {
            Actor->SetActorTransform(Transform);
            Actor->SetOwner(Owner);
            Actor->SetInstigator(Instigator);
            Actor->SetActorHiddenInGame(false);
            Actor->SetActorEnableCollision(true);
            Actor->SetActorTickEnabled(true);

            if (AGeoProjectile* Proj = Cast<AGeoProjectile>(Actor))
            {
                if (UProjectileMovementComponent* Move = Proj->ProjectileMovement)
                {
                    Move->StopMovementImmediately();
                }
                Proj->OnPooledSpawned();
            }
            return Actor;
        }
    }

    // Spawn new
    FActorSpawnParameters Params;
    Params.Owner = Owner;
    Params.Instigator = Instigator;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    AActor* NewActor = World->SpawnActor<AActor>(Class, Transform, Params);
    if (AGeoProjectile* Proj = Cast<AGeoProjectile>(NewActor))
    {
        if (UProjectileMovementComponent* Move = Proj->ProjectileMovement)
        {
            Move->StopMovementImmediately();
        }
        Proj->OnPooledSpawned();
    }
    return NewActor;
}

AGeoProjectile* UGeoActorPoolSubsystem::SpawnPooledProjectile(const UObject* WorldContextObject, TSubclassOf<AGeoProjectile> Class, const FTransform& Transform)
{
    if (!Class)
    {
        return nullptr;
    }
    AActor* Spawned = SpawnPooledClass(*Class, Transform, /*Owner*/nullptr, /*Instigator*/nullptr);
    return Cast<AGeoProjectile>(Spawned);
}

void UGeoActorPoolSubsystem::ReleaseActor(AActor* Actor)
{
    if (!IsValid(Actor)) return;

    // Custom handling for projectiles
    if (AGeoProjectile* Proj = Cast<AGeoProjectile>(Actor))
    {
        if (UProjectileMovementComponent* Move = Proj->ProjectileMovement)
        {
            Move->StopMovementImmediately();
        }
        Proj->OnPooledDespawned();
    }

    Actor->SetActorEnableCollision(false);
    Actor->SetActorTickEnabled(false);
    Actor->SetActorHiddenInGame(true);

    // Return to pool
    InactiveByClass.FindOrAdd(Actor->GetClass()).Add(Actor);
}
