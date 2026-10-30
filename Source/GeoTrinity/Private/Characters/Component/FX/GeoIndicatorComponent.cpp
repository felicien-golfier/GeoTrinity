// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/FX/GeoIndicatorComponent.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Settings/GameDataSettings.h"
#include "TimerManager.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
void FGeoIndicatorItem::PostReplicatedAdd(FGeoIndicatorArray const& InArraySerializer)
{
	ZoneVisual = InArraySerializer.Owner->SpawnOwnIndicator(State);
}

void FGeoIndicatorItem::PreReplicatedRemove(FGeoIndicatorArray const& /*InArraySerializer*/)
{
	if (IsValid(ZoneVisual))
	{
		ZoneVisual->DestroyComponent();
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
		if (IsValid(Item.ZoneVisual))
		{
			Item.ZoneVisual->DestroyComponent();
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
		State.StartServerTime = FMath::Clamp(
			State.StartServerTime, ServerTime - GetDefault<UGameDataSettings>()->MaxLatencyCompensation, ServerTime);
	}

	float const RemainingTime = State.StartServerTime + State.Duration - ServerTime;
	if (State.Duration > 0.f && RemainingTime <= 0.f)
	{
		return INDEX_NONE;
	}

	FGeoIndicatorItem& Item = Indicators.Items.AddDefaulted_GetRef();
	Item.State = MoveTemp(State);
	Item.ZoneVisual = SpawnOwnIndicator(Item.State);
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
	int32 const Index = Indicators.Items.IndexOfByPredicate(
		[Handle](FGeoIndicatorItem const& Item)
		{
			return Item.ReplicationID == Handle;
		});
	if (Index != INDEX_NONE)
	{
		if (IsValid(Indicators.Items[Index].ZoneVisual))
		{
			Indicators.Items[Index].ZoneVisual->DestroyComponent();
		}

		Indicators.Items.RemoveAt(Index);
		Indicators.MarkArrayDirty();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoZoneVisualComponent* UGeoIndicatorComponent::SpawnOwnIndicator(FGeoIndicatorState const& State) const
{
	return SpawnIndicator(this, State, State.bAttachToOwner ? GetOwner()->GetRootComponent() : nullptr);
}

UGeoZoneVisualComponent* UGeoIndicatorComponent::SpawnIndicator(UObject const* WorldContextObject,
																FGeoIndicatorState const& State,
																USceneComponent* AttachParent)
{
	UWorld* const World = WorldContextObject->GetWorld();
	float const ElapsedTime = FMath::Max(0.f, GeoLib::GetServerTime(World, true) - State.StartServerTime);
	if (GeoLib::IsDedicatedServer(World) || (State.Duration > 0.f && ElapsedTime >= State.Duration))
	{
		return nullptr;
	}

	// Owned by the actor it follows, else by the world, like any free-standing effect.
	UObject* const Outer = AttachParent ? static_cast<UObject*>(AttachParent->GetOwner()) : World;
	UGeoZoneVisualComponent* const Component = NewObject<UGeoZoneVisualComponent>(Outer);
	Component->Layer = EGeoZoneLayer::Telegraph;
	Component->ColorMask = State.ColorMask;
	if (State.Shape == EGeoIndicatorShape::Round)
	{
		Component->Shape.Shape = EGeoZoneShape::Circle;
		Component->Shape.Radius = State.Size.X;
	}
	else
	{
		Component->Shape.Shape = EGeoZoneShape::Beam;
		Component->Shape.Length = State.Size.X;
		Component->Shape.HalfWidth = State.Size.Y;
	}

	Component->SetTimedFill(State.StartServerTime, State.Duration);
	if (AttachParent)
	{
		Component->SetupAttachment(AttachParent);
		Component->SetRelativeLocation(State.Location);
	}
	else
	{
		Component->SetWorldLocation(State.Location);
	}

	Component->RegisterComponentWithWorld(World);
	if (State.Duration > 0.f)
	{
		FTimerHandle EndTimerHandle;
		World->GetTimerManager().SetTimer(EndTimerHandle,
										  FTimerDelegate::CreateWeakLambda(Component,
																		   [Component]
																		   {
																			   Component->DestroyComponent();
																		   }),
										  State.Duration - ElapsedTime, false);
	}

	return Component;
}
