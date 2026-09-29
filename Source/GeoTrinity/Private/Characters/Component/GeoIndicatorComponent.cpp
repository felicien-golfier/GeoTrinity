// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoIndicatorComponent.h"

#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoNiagaraParams.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void FGeoIndicatorItem::PostReplicatedAdd(FGeoIndicatorArray const& InArraySerializer)
{
	NiagaraComponent = InArraySerializer.Owner->SpawnOwnIndicator(State);
}

void FGeoIndicatorItem::PreReplicatedRemove(FGeoIndicatorArray const& /*InArraySerializer*/)
{
	if (IsValid(NiagaraComponent))
	{
		NiagaraComponent->DestroyComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoIndicatorComponent::UGeoIndicatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGeoIndicatorComponent::OnRegister()
{
	Super::OnRegister();
	Indicators.Owner = this;
}

void UGeoIndicatorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGeoIndicatorComponent, Indicators, COND_SkipOwner);
}

void UGeoIndicatorComponent::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	for (FGeoIndicatorItem const& Item : Indicators.Items)
	{
		if (IsValid(Item.NiagaraComponent))
		{
			Item.NiagaraComponent->DestroyComponent();
		}
	}

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoIndicatorComponent::AddIndicator(FGeoIndicatorState State)
{
	float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
	if (GeoLib::IsServer(GetWorld()))
	{
		State.StartServerTime =
			FMath::Clamp(State.StartServerTime,
						 ServerTime - GetDefault<UGameDataSettings>()->MaxLatencyCompensation, ServerTime);
	}

	float const RemainingTime = State.StartServerTime + State.Duration - ServerTime;
	if (State.Duration > 0.f && RemainingTime <= 0.f)
	{
		return INDEX_NONE;
	}

	FGeoIndicatorItem& Item = Indicators.Items.AddDefaulted_GetRef();
	Item.State = MoveTemp(State);
	Item.NiagaraComponent = SpawnOwnIndicator(Item.State);
	Indicators.MarkItemDirty(Item);

	int32 const Handle = Item.ReplicationID;
	if (Item.State.Duration > 0.f)
	{
		FTimerHandle ExpiryTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			ExpiryTimerHandle, FTimerDelegate::CreateUObject(this, &UGeoIndicatorComponent::RemoveIndicator, Handle),
			RemainingTime, false);
	}

	return Handle;
}

void UGeoIndicatorComponent::RemoveIndicator(int32 const Handle)
{
	int32 const Index =
		Indicators.Items.IndexOfByPredicate([Handle](FGeoIndicatorItem const& Item)
											{ return Item.ReplicationID == Handle; });
	if (Index != INDEX_NONE)
	{
		if (IsValid(Indicators.Items[Index].NiagaraComponent))
		{
			Indicators.Items[Index].NiagaraComponent->DestroyComponent();
		}

		Indicators.Items.RemoveAt(Index);
		Indicators.MarkArrayDirty();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UNiagaraComponent* UGeoIndicatorComponent::SpawnOwnIndicator(FGeoIndicatorState const& State) const
{
	return SpawnIndicator(this, State, State.bAttachToOwner ? GetOwner()->GetRootComponent() : nullptr);
}

UNiagaraComponent* UGeoIndicatorComponent::SpawnIndicator(UObject const* WorldContextObject,
														  FGeoIndicatorState const& State,
														  USceneComponent* AttachParent)
{
	UWorld const* const World = WorldContextObject->GetWorld();
	float const ElapsedTime = FMath::Max(0.f, GeoLib::GetServerTime(World, true) - State.StartServerTime);
	if (GeoLib::IsDedicatedServer(World) || (State.Duration > 0.f && ElapsedTime >= State.Duration))
	{
		return nullptr;
	}

	UGameDataSettings const* const GDSettings = GetDefault<UGameDataSettings>();
	bool const bIsRound = State.Shape == EGeoIndicatorShape::Round;
	UNiagaraSystem* const System =
		GDSettings->GetLoadedDataAsset(bIsRound ? GDSettings->RoundIndicatorSystem : GDSettings->RayIndicatorSystem);
	if (!ensureMsgf(System, TEXT("%hs: no %s indicator system in Game Data Settings"), __FUNCTION__,
					*UEnum::GetValueAsString(State.Shape)))
	{
		return nullptr;
	}

	UNiagaraComponent* const Component =
		AttachParent
		? UNiagaraFunctionLibrary::SpawnSystemAttached(System, AttachParent, NAME_None, State.Location,
													   FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
													   /*bAutoDestroy*/ true, /*bAutoActivate*/ false)
		: UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, System, State.Location,
														 FRotator::ZeroRotator, FVector(1.f), /*bAutoDestroy*/ true,
														 /*bAutoActivate*/ false);
	if (Component)
	{
		Component->SetVariableFloat(GeoNiagaraParams::Lifetime, State.Duration);
		if (bIsRound)
		{
			Component->SetVariableFloat(GeoNiagaraParams::Radius, State.Size.X);
		}
		else
		{
			Component->SetVariableFloat(GeoNiagaraParams::BeamLength, State.Size.X);
			Component->SetVariableFloat(GeoNiagaraParams::BeamWidth, State.Size.Y);
		}

		GeoNiagaraParams::SetMeaningColors(Component, State.Colors);
		Component->Activate();
		Component->AdvanceSimulationByTime(ElapsedTime, World->GetDeltaSeconds());
	}

	return Component;
}
