#include "Actor/Deployable/GeoDeployableBase.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase::AGeoDeployableBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnRecalled()
{
	OnDeployableExpired();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float AGeoDeployableBase::GetDurationPercent() const
{
	return FMath::Clamp(RemainingDuration / GetData()->MaxDuration, 0.f, 1.f);
}

void AGeoDeployableBase::InitInteractableData(FInteractableActorData* InputData)
{
	Super::InitInteractableData(InputData);
	ensureMsgf(GetData()->MaxDuration > 0.f,
			   TEXT("Deployable has no duration ! It must have been missed init, or data has lost in the way"));
	RemainingDuration = GetData()->MaxDuration;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bExpired || GetData()->MaxDuration <= 0.f)
	{
		return;
	}

	RemainingDuration -= DeltaSeconds;
	if (RemainingDuration <= 0.f)
	{
		RemainingDuration = 0.f;
		OnDeployableExpired();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnHealthChanged(float NewValue)
{
	if (NewValue <= 0.f && !bExpired)
	{
		OnDeployableExpired();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnDeployableExpired()
{
	if (bExpired)
	{
		return;
	}
	bExpired = true;

	OnDeployableDestroyed.Broadcast(this);
	Destroy();
}
