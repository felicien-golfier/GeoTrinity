#include "Actor/Deployable/GeoDeployableBase.h"

#include "System/GeoActorPoolingSubsystem.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase::AGeoDeployableBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Init()
{
	bExpired = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::End()
{
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	DeployableData = nullptr;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::InitDeployableData(FDeployableData* Data)
{
	DeployableData = Data;
	InitInteractableData(Data);
	RemainingDuration = Data->MaxDuration;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnRecalled()
{
	OnDeployableExpired();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float AGeoDeployableBase::GetDurationPercent() const
{
	if (!DeployableData || DeployableData->MaxDuration <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp(RemainingDuration / DeployableData->MaxDuration, 0.f, 1.f);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bExpired || !DeployableData || DeployableData->MaxDuration <= 0.f)
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

	if (UGeoActorPoolingSubsystem* Pool = UGeoActorPoolingSubsystem::Get(GetWorld()))
	{
		Pool->ReleaseActor(this);
	}
}
