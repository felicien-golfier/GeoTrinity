#include "Characters/Component/GeoDeployableManagerComponent.h"

#include "Actor/Deployable/GeoDeployableBase.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
UGeoDeployableManagerComponent::UGeoDeployableManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::RegisterDeployable(AGeoDeployableBase* Deployable)
{
	if (!Deployable || Deployables.Contains(Deployable))
	{
		return;
	}

	// If at max, recall the oldest to make room
	if (Deployables.Num() >= MaxDeployables && Deployables.Num() > 0)
	{
		RecallOldest();
	}

	Deployables.Add(Deployable);
	Deployable->OnDeployableDestroyed.AddDynamic(this, &ThisClass::OnDeployableDestroyed);

	OnDeployCountChanged.Broadcast(Deployables.Num(), MaxDeployables);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::RecallAll()
{
	// Copy since OnRecalled will trigger removal via the delegate
	TArray<TObjectPtr<AGeoDeployableBase>> Copy = Deployables;
	for (AGeoDeployableBase* Deployable : Copy)
	{
		if (IsValid(Deployable))
		{
			Deployable->OnRecalled();
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::RecallOldest()
{
	if (Deployables.Num() > 0 && IsValid(Deployables[0]))
	{
		Deployables[0]->OnRecalled();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float UGeoDeployableManagerComponent::GetDeployRatio() const
{
	if (MaxDeployables <= 0)
	{
		return 0.f;
	}
	return static_cast<float>(Deployables.Num()) / static_cast<float>(MaxDeployables);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::OnDeployableDestroyed(AGeoDeployableBase* Deployable)
{
	Deployables.Remove(Deployable);
	OnDeployCountChanged.Broadcast(Deployables.Num(), MaxDeployables);
}
