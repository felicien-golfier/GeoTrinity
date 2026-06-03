#include "Characters/Component/GeoDeployableManagerComponent.h"

#include "Actor/Deployable/GeoDeployableBase.h"
#include "GeoTrinity/GeoTrinity.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
UGeoDeployableManagerComponent::UGeoDeployableManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
bool UGeoDeployableManagerComponent::CanDeploy(TSubclassOf<AGeoDeployableBase> DeployableClass)
{
	if (!DeployableClass)
	{
		ensureMsgf(false, TEXT("GeoDeployableManagerComponent: Tried to deploy a null class."));
		return false;
	}

	if (int32 const* ClassMax = DeployableSlots.Find(DeployableClass))
	{
		FDeployableBucket const* Bucket = Deployables.Find(DeployableClass);
		return Bucket->Deployables.Num() == 0 || Bucket->Deployables.Num() < *ClassMax;
	}

	return Deployables.FindOrAdd(DeployableClass).Deployables.Num() < MaxDeployables;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
TArray<AGeoDeployableBase*> UGeoDeployableManagerComponent::GetAllDeployables() const
{
	TArray<AGeoDeployableBase*> All;
	for (auto const& [Class, Bucket] : Deployables)
	{
		All.Append(Bucket.Deployables);
	}
	return All;
}

void UGeoDeployableManagerComponent::SetDeployableInfinitCount(TSubclassOf<AGeoDeployableBase> const Class)
{
	DeployableSlots.FindOrAdd(Class) = 0;
}

void UGeoDeployableManagerComponent::RemoveInvalidDeployables(FDeployableBucket& Bucket)
{
	for (int i = Bucket.Deployables.Num() - 1; i >= 0; --i)
	{
		if (!IsValid(Bucket.Deployables[i]))
		{
			UE_LOG(LogGeoTrinity, Warning, TEXT("Deployable Invalid in the array"));
			Bucket.Deployables.RemoveAt(i);
		}
	}
}
// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::RegisterDeployable(AGeoDeployableBase* Deployable)
{
	if (!IsValid(Deployable))
	{
		return;
	}

	FDeployableBucket& Bucket = Deployables.FindOrAdd(Deployable->GetClass());
	if (Bucket.Deployables.Contains(Deployable))
	{
		ensureMsgf(false, TEXT("GeoDeployableManagerComponent: Tried to register '%s' twice."),
				   *GetNameSafe(Deployable));
		return;
	}

	RemoveInvalidDeployables(Bucket);

	if (!CanDeploy(Deployable->GetClass()))
	{
		UE_LOG(LogTemp, Error,
			   TEXT("GeoDeployableManagerComponent: Tried to register '%s' but already at max. "
					"Deploy ability should have been blocked by CanActivateAbility."),
			   *GetNameSafe(Deployable));
	}

	Bucket.Deployables.Add(Deployable);
	Deployable->OnDeployableExpiredEvent.AddDynamic(this, &ThisClass::OnDeployableDestroyed);
	// Ensure we remove it also on client even if forced expired by server
	Deployable->OnDestroyed.AddDynamic(this, &ThisClass::OnDeployableDestroyed);
	OnDeployCountChanged.Broadcast(Bucket.Deployables.Num(), MaxDeployables);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::ForceExpireAll() const
{
	// Copy since Recall will trigger removal via the delegate
	TArray<AGeoDeployableBase*> const Copy = GetAllDeployables();
	for (AGeoDeployableBase* Deployable : Copy)
	{
		if (IsValid(Deployable))
		{
			Deployable->Expire(0.f);
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::OnDeployableDestroyed(AActor* Deployable)
{
	OnDeployableDestroyed(Cast<AGeoDeployableBase>(Deployable));
}

void UGeoDeployableManagerComponent::OnDeployableDestroyed(AGeoDeployableBase* Deployable)
{
	FDeployableBucket& Bucket = Deployables.FindOrAdd(Deployable->GetClass());
	Bucket.Deployables.Remove(Deployable);

	OnDeployCountChanged.Broadcast(Bucket.Deployables.Num(), MaxDeployables);
}
