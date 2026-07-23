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
bool UGeoDeployableManagerComponent::CanDeploy(TSubclassOf<AGeoDeployableBase> const DeployableClass)
{
	return DeployableClass->GetDefaultObject<AGeoDeployableBase>()->DestroyOldestWhenLimitReached()
		|| !HasReachMaxLimit(DeployableClass);
}

bool UGeoDeployableManagerComponent::HasReachMaxLimit(TSubclassOf<AGeoDeployableBase> const DeployableClass)
{
	if (!DeployableClass)
	{
		ensureMsgf(false, TEXT("GeoDeployableManagerComponent: Tried to deploy a null class."));
		return true;
	}

	// Class-level (CDO) property, agreed on by every machine without needing DeployableSlots to replicate.
	if (DeployableClass->GetDefaultObject<AGeoDeployableBase>()->IsUnlimitedDeploy())
	{
		return false;
	}

	if (int32 const* ClassMax = DeployableSlots.Find(DeployableClass))
	{
		if (*ClassMax <= 0) // 0 = unlimited
		{
			return false;
		}
		FDeployableBucket const* Bucket = Deployables.Find(DeployableClass);
		return Bucket && Bucket->Deployables.Num() >= *ClassMax;
	}

	return Deployables.FindOrAdd(DeployableClass).Deployables.Num() >= MaxDeployables;
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

TArray<AGeoDeployableBase*> UGeoDeployableManagerComponent::GetDeployables(
	TSubclassOf<AGeoDeployableBase> const Class) const
{
	TArray<AGeoDeployableBase*> Result;
	if (!Class)
	{
		return Result;
	}

	for (auto const& [StoredClass, Bucket] : Deployables)
	{
		if (StoredClass->IsChildOf(Class))
		{
			Result.Append(Bucket.Deployables);
		}
	}
	return Result;
}

void UGeoDeployableManagerComponent::SetDeployableInfinitCount(TSubclassOf<AGeoDeployableBase> const Class)
{
	DeployableSlots.FindOrAdd(Class) = 0;
}

void UGeoDeployableManagerComponent::RemoveDeployableSlot(TSubclassOf<AGeoDeployableBase> const Class)
{
	DeployableSlots.Remove(Class);
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

	if (HasReachMaxLimit(Deployable->GetClass()))
	{
		if (Deployable->DestroyOldestWhenLimitReached())
		{
			checkf(Bucket.Deployables.Num() > 0,
				   TEXT("Deployables reach the max limit but their is nothing in the array."));
			// Expire() synchronously broadcasts OnDeployableExpiredEvent, which routes to OnDeployableDestroyed and
			// removes the oldest from the bucket. Do NOT also RemoveAt(0) here — that would drop a second, still-alive
			// deployable from tracking, so the limit would only visibly enforce every other spawn.
			Bucket.Deployables[0]->Expire();
		}
		else
		{
			// TODO : Can happen between the time a deployable projectile is spawned and deployable is here.
			UE_LOG(LogTemp, Error,
				   TEXT("GeoDeployableManagerComponent: Tried to register '%s' but already at max. "
						"Deploy ability should have been blocked by CanActivateAbility."),
				   *GetNameSafe(Deployable));
		}
	}

	Bucket.Deployables.Add(Deployable);
	// Use OnDestroy to ensure we remove it also on client even if forced expired by server.
	// And keep in array even if blinking
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
	// Reached by both OnDeployableExpiredEvent (Expire, authority-only, fires early) and OnDestroyed (engine, every
	// machine). On a listen host both fire for the same actor, so only act on the invocation that actually removes it —
	// otherwise the count broadcasts twice and transiently reads wrong.
	FDeployableBucket& Bucket = Deployables.FindOrAdd(Deployable->GetClass());
	if (Bucket.Deployables.Remove(Deployable) == 0)
	{
		return;
	}

	OnDeployCountChanged.Broadcast(Bucket.Deployables.Num(), MaxDeployables);
}
