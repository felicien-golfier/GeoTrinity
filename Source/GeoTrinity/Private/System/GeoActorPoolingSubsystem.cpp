// GeoActorPoolSubsystem.cpp

#include "System/GeoActorPoolingSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "System/GeoPoolableInterface.h"

AActor* UGeoActorPoolingSubsystem::PopWithClass(UClass* Class, FTransform const& Transform, AActor* Owner,
												APawn* Instigator, bool bInit, bool bActivate)
{
	if (!ensureMsgf(Class, TEXT("PopWithClass called with invalid Class")))
	{
		return nullptr;
	}

	// Try reuse from pool
	TArray<TWeakObjectPtr<AActor>>& PoolForClass = Pool.FindOrAdd(Class);

	AActor* Actor = nullptr;
	while (PoolForClass.Num() > 0)
	{
		TWeakObjectPtr<AActor> Weak = PoolForClass.Pop();
		if (Weak.IsValid())
		{
			Actor = Weak.Get();
			break;
		}
		UE_LOG(LogTemp, Error, TEXT("[Pool] Discarding stale weak pointer for %s! Actor was GC'd?"), *Class->GetName());
	}

	if (!Actor)
	{
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.Instigator = Instigator;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Actor = SpawnActor(Class, Params);
	}

	if (!Actor)
	{
		return nullptr;
	}

	Actor->SetOwner(Owner);
	Actor->SetInstigator(Instigator);
	Actor->TeleportTo(Transform.GetLocation(), Transform.GetRotation().Rotator(), false, true);

	if (bActivate)
	{
		ChangeActorState(Actor, true);
	}

	if (Actor->GetIsReplicated())
	{
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

void UGeoActorPoolingSubsystem::PreSpawn(UClass* Class, uint16 const Count, AActor* Owner, APawn* Instigator)
{
	if (!ensureMsgf(Class && Count > 0, TEXT("PreSpawn requires a valid Class and a Count greater than 0")))
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Instigator;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<TWeakObjectPtr<AActor>>& PoolByClass = Pool.FindOrAdd(Class);
	int32 const MissingCount = Count - PoolByClass.Num();
	PoolByClass.Reserve(Count);

	for (int32 i = 0; i < MissingCount; ++i)
	{
		if (AActor* NewActor = SpawnActor(Class, Params))
		{
			PoolByClass.Add(NewActor);
		}
	}
}

bool UGeoActorPoolingSubsystem::GetActorState(AActor const* Actor)
{
	return Actor->GetActorEnableCollision() && !Actor->IsHidden();
}

void UGeoActorPoolingSubsystem::ChangeActorState(AActor* NewActor, bool bActive)
{
	NewActor->SetActorEnableCollision(bActive);
	NewActor->SetActorTickEnabled(bActive);
	NewActor->ForEachComponent<UActorComponent>(false, [bActive](UActorComponent* Component)
												{ Component->SetComponentTickEnabled(bActive); });
	NewActor->SetActorHiddenInGame(!bActive);

	NewActor->SetNetDormancy(bActive ? DORM_Awake : DORM_DormantAll);
	NewActor->FlushNetDormancy();
}

AActor* UGeoActorPoolingSubsystem::SpawnActor(UClass* Class, FActorSpawnParameters const& Params)
{

	UWorld* World = GetWorld();
	ensureMsgf(World, TEXT("World is invalid"));

	AActor* NewActor = World->SpawnActor<AActor>(Class, FTransform::Identity, Params);
	if (!ensureMsgf(NewActor, TEXT("Failed to spawn actor of class %s"), *Class->GetName()))
	{
		return nullptr;
	}

	ChangeActorState(NewActor, false);

	// An actor spawned before the world begins play (PreSpawn from a pattern's InitializeComponent) registers its tick
	// functions at its own BeginPlay, after the deactivation above, and registration ORs bStartWithTickEnabled back
	// into the enable state (AActor::RegisterActorTickFunctions, UActorComponent::SetupActorComponentTickFunction).
	// The flag is only read there, so clearing it once here is what makes the deactivation stick.
	NewActor->PrimaryActorTick.bStartWithTickEnabled = false;
	NewActor->ForEachComponent<UActorComponent>(false, [](UActorComponent* Component)
												{ Component->PrimaryComponentTick.bStartWithTickEnabled = false; });

	return NewActor;
}

void UGeoActorPoolingSubsystem::ReleaseActor(AActor* Actor)
{
	if (!ensureMsgf(IsValid(Actor), TEXT("[Pool] ReleaseActor called with invalid actor!")))
	{
		return;
	}

	if (!ensureMsgf(GetActorState(Actor), TEXT("[Pool] ReleaseActor called twice on %s"), *Actor->GetName()))
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
UGeoActorPoolingSubsystem* UGeoActorPoolingSubsystem::Get(UWorld const* World)
{
	UGeoActorPoolingSubsystem* Pool = World->GetSubsystem<UGeoActorPoolingSubsystem>();
	ensureMsgf(Pool, TEXT("GeoActorPoolingSubsystem is invalid!"));
	return Pool;
}

UGeoActorPoolingSubsystem* UGeoActorPoolingSubsystem::Get(UObject const* WorldContextObject)
{
	return Get(WorldContextObject->GetWorld());
}
