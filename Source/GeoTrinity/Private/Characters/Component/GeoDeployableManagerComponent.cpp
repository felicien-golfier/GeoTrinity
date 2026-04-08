#include "Characters/Component/GeoDeployableManagerComponent.h"

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Net/UnrealNetwork.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
UGeoDeployableManagerComponent::UGeoDeployableManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGeoDeployableManagerComponent, Deployables);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoDeployableManagerComponent::RegisterDeployable(AGeoDeployableBase* Deployable)
{
	if (!Deployable || Deployables.Contains(Deployable))
	{
		return;
	}

	checkf(Deployables.Num() < MaxDeployables,
		   TEXT("GeoDeployableManagerComponent: Tried to register a deployable but already at max (%d/%d). "
				"Deploy ability should have been blocked by CanActivateAbility."),
		   Deployables.Num(), MaxDeployables);

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
