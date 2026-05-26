#include "Characters/Component/GeoDeployableManagerComponent.h"

#include "Actor/Deployable/GeoDeployableBase.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
UGeoDeployableManagerComponent::UGeoDeployableManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
bool UGeoDeployableManagerComponent::CanDeploy(TSubclassOf<AGeoDeployableBase> DeployableClass) const
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

	return GetDeployedCount() < MaxDeployables;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
int32 UGeoDeployableManagerComponent::GetDeployedCount() const
{
	int32 Total = 0;
	for (auto const& [Class, Bucket] : Deployables)
	{
		Total += Bucket.Deployables.Num();
	}
	return Total;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
TArray<AGeoDeployableBase*> UGeoDeployableManagerComponent::GetDeployables() const
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

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::RegisterDeployable(AGeoDeployableBase* Deployable)
{
	if (!IsValid(Deployable))
	{
		return;
	}

	auto& [Bucket] = Deployables.FindOrAdd(Deployable->GetClass());
	if (Bucket.Contains(Deployable))
	{
		ensureMsgf(false, TEXT("GeoDeployableManagerComponent: Tried to register '%s' twice."),
				   *GetNameSafe(Deployable));
		return;
	}

	if (!CanDeploy(Deployable->GetClass()))
	{
		UE_LOG(LogTemp, Error,
			   TEXT("GeoDeployableManagerComponent: Tried to register '%s' but already at max. "
					"Deploy ability should have been blocked by CanActivateAbility."),
			   *GetNameSafe(Deployable));
	}

	Bucket.Add(Deployable);
	Deployable->OnDeployableExpiredEvent.AddDynamic(this, &ThisClass::OnDeployableDestroyed);
	// Ensure we remove it also on client even if forced expired by server
	Deployable->OnDestroyed.AddDynamic(this, &ThisClass::OnDeployableDestroyed);
	OnDeployCountChanged.Broadcast(GetDeployedCount(), MaxDeployables);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::ExpireAll()
{
	// Copy since Recall will trigger removal via the delegate
	TArray<AGeoDeployableBase*> const Copy = GetDeployables();
	for (AGeoDeployableBase* Deployable : Copy)
	{
		if (IsValid(Deployable))
		{
			Deployable->Expire();
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float UGeoDeployableManagerComponent::GetDeployRatio() const
{
	if (MaxDeployables <= 0)
	{
		return 0.f;
	}
	return static_cast<float>(GetDeployedCount()) / static_cast<float>(MaxDeployables);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::OnDeployableDestroyed(AActor* Deployable)
{
	OnDeployableDestroyed(Cast<AGeoDeployableBase>(Deployable));
}

void UGeoDeployableManagerComponent::OnDeployableDestroyed(AGeoDeployableBase* Deployable)
{
	if (FDeployableBucket* Bucket = Deployables.Find(Deployable->GetClass()))
	{
		Bucket->Deployables.Remove(Deployable);
	}
	OnDeployCountChanged.Broadcast(GetDeployedCount(), MaxDeployables);
}
