// GeoActorPoolSubsystem.cpp

#include "System/GeoActorPoolingSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "System/GeoPoolableInterface.h"

AActor* UGeoActorPoolingSubsystem::PopWithClass(UClass* Class, const FTransform& Transform, AActor* Owner,
	APawn* Instigator, bool bInit)
{
	if (!ensure(Class))
	{
		return nullptr;
	}

	// Try reuse from pool
	TArray<TWeakObjectPtr<AActor>>& PoolForClass = Pool.FindOrAdd(Class);

	if (PoolForClass.Num() == 0)
	{
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.Instigator = Instigator;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnActor(Class, Params);
	}

	TWeakObjectPtr<AActor> Weak = PoolForClass.Pop();
	AActor* Actor = Weak.Get();
	checkf(IsValid(Actor), TEXT("Weak Ptr from pool is empty !"));

	Actor->SetOwner(Owner);
	Actor->SetInstigator(Instigator);
	Actor->TeleportTo(Transform.GetLocation(), Transform.GetRotation().Rotator(), false, true);
	ChangeActorState(Actor, true);

	// Force replication update for networked actors
	if (Actor->GetIsReplicated())
	{
		// Force a net update
		Actor->ForceNetUpdate();
	}

	if (bInit)
	{
		if (IGeoPoolableInterface* Poolable = Cast<IGeoPoolableInterface>(Actor))
		{
			Poolable->Init();
		}
	}

	return Actor;
}

void UGeoActorPoolingSubsystem::PreSpawn(UClass* Class, const uint16 Count, AActor* Owner, APawn* Instigator)
{
	UWorld* World = GetWorld();
	checkf(World, TEXT("World is invalid"));

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Instigator;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<TWeakObjectPtr<AActor>>& PoolByClass = Pool.FindOrAdd(Class);

	PoolByClass.Reserve(PoolByClass.Num() + Count);

	for (int32 i = 0; i < Count; ++i)
	{
		SpawnActor(Class, Params);
	}
}

void UGeoActorPoolingSubsystem::ChangeActorState(AActor* NewActor, bool bActive)
{
	// Make inactive
	NewActor->SetActorEnableCollision(bActive);
	NewActor->SetActorTickEnabled(bActive);
	NewActor->SetActorHiddenInGame(!bActive);

	NewActor->SetNetDormancy(bActive ? DORM_Awake : DORM_DormantAll);
	NewActor->FlushNetDormancy();
}

AActor* UGeoActorPoolingSubsystem::SpawnActor(UClass* Class, const FActorSpawnParameters& Params)
{

	UWorld* World = GetWorld();
	checkf(World, TEXT("World is invalid"));

	AActor* NewActor = World->SpawnActor<AActor>(Class, FTransform::Identity, Params);
	checkf(NewActor, TEXT("Failed to spawn actor of class %s"), *Class->GetName());

	ChangeActorState(NewActor, false);

	Pool.FindOrAdd(Class).Add(NewActor);
	return NewActor;
}

void UGeoActorPoolingSubsystem::ReleaseActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (IGeoPoolableInterface* Poolable = Cast<IGeoPoolableInterface>(Actor))
	{
		Poolable->End();
	}

	ChangeActorState(Actor, false);

	// Return to pool
	Pool.FindOrAdd(Actor->GetClass()).Add(Actor);
}
UGeoActorPoolingSubsystem* UGeoActorPoolingSubsystem::Get(const UWorld* World)
{
	UGeoActorPoolingSubsystem* Pool = World->GetSubsystem<UGeoActorPoolingSubsystem>();
	ensureMsgf(Pool, TEXT("GeoActorPoolingSubsystem is invalid!"));
	return Pool;
}

UGeoActorPoolingSubsystem* UGeoActorPoolingSubsystem::Get(const UObject* WorldContextObject)
{
	return Get(WorldContextObject->GetWorld());
}
