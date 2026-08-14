// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoBackgroundPulseComponent.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/GeoGameCamera.h"

namespace
{
	constexpr int32 MaxPulseSlots = 8;
	constexpr FLinearColor UnusedSlotValue(0.f, 0.f, 0.f, 0.f);

	FName GetSlotParameterName(int32 const SlotIndex)
	{
		return FName(FString::Printf(TEXT("PulseSource_%02d"), SlotIndex));
	}

	FVector2D RandomUnitVector()
	{
		float const Angle = FMath::FRandRange(0.f, 2.f * UE_PI);
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}
} // namespace

UGeoBackgroundPulseComponent::UGeoBackgroundPulseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGeoBackgroundPulseComponent::BeginPlay()
{
	Super::BeginPlay();

	AuthoredMode = Mode;

	if (GeoLib::IsDedicatedServer(this)
		|| !ensureMsgf(PulseCollection, TEXT("%s: no PulseCollection — the background lattice never pulses"),
					   *GetOwner()->GetName()))
	{
		SetComponentTickEnabled(false);
		return;
	}

	// The collection is global state that outlives any single writer, so every slot starts from a known zero —
	// otherwise a PulseCount below eight leaves whatever the previous session wrote lit and stationary.
	for (int32 SlotIndex = 0; SlotIndex < MaxPulseSlots; ++SlotIndex)
	{
		SetSlot(SlotIndex, UnusedSlotValue);
	}

	FVector2D const Center(GetOwner()->GetActorLocation());
	Pulses.SetNum(FMath::Min(PulseCount, MaxPulseSlots));
	for (int32 SlotIndex = 0; SlotIndex < Pulses.Num(); ++SlotIndex)
	{
		FGeoPulse& Pulse = Pulses[SlotIndex];
		Pulse.Location = Center + RandomUnitVector() * FMath::FRand() * AreaRadius;
		Pulse.Direction = RandomUnitVector();
		// Spread the starting phases evenly so the rings never expand in unison, which reads as one thick pulse
		// rather than several independent ones.
		Pulse.RingRadius = MaxRingRadius * SlotIndex / Pulses.Num();
	}
}

UGeoBackgroundPulseComponent* UGeoBackgroundPulseComponent::Get(UObject const* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<AGeoGameCamera> It(World);
	if (!It)
	{
		return nullptr;
	}
	return It->FindComponentByClass<UGeoBackgroundPulseComponent>();
}

void UGeoBackgroundPulseComponent::TickComponent(float DeltaTime, ELevelTick TickType,
												 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TArray<AActor*> TrackedActors;
	if (Mode == EGeoPulseMode::Actors)
	{
		TrackedActors = GatherTrackedActors();
	}

	for (int32 SlotIndex = 0; SlotIndex < Pulses.Num(); ++SlotIndex)
	{
		FGeoPulse& Pulse = Pulses[SlotIndex];
		Pulse.RingRadius = FMath::Fmod(Pulse.RingRadius + RingSpeed * DeltaTime, MaxRingRadius);

		if (Mode == EGeoPulseMode::Actors)
		{
			if (!TrackedActors.IsValidIndex(SlotIndex))
			{
				SetSlot(SlotIndex, UnusedSlotValue);
				continue;
			}
			Pulse.Location = FVector2D(TrackedActors[SlotIndex]->GetActorLocation());
		}
		else
		{
			MovePulse(Pulse, DeltaTime);
		}

		SetSlot(SlotIndex, FLinearColor(Pulse.Location.X, Pulse.Location.Y, Pulse.RingRadius,
										1.f - Pulse.RingRadius / MaxRingRadius));
	}
}

TArray<AActor*> UGeoBackgroundPulseComponent::GatherTrackedActors() const
{
	FVector2D const Center(GetOwner()->GetActorLocation());
	TArray<AActor*> TrackedActors = GeoASLib::GetInteractableActors(this, FGenericTeamId::NoTeam, TeamAttitudeMask::All,
																	false, Center, AreaRadius);
	TrackedActors.Sort(
		[Center](AActor const& Left, AActor const& Right)
		{
			return FVector2D::DistSquared(Center, FVector2D(Left.GetActorLocation()))
				 < FVector2D::DistSquared(Center, FVector2D(Right.GetActorLocation()));
		});
	return TrackedActors;
}

void UGeoBackgroundPulseComponent::MovePulse(FGeoPulse& Pulse, float DeltaTime) const
{
	if (Mode == EGeoPulseMode::Wander)
	{
		// Plain FMath RNG rather than a seeded stream: this drives nothing but pixels, so the ability rule about
		// deriving every draw from StoredPayload.Seed has nothing to keep in sync here.
		Pulse.Direction = Pulse.Direction.GetRotated(FMath::FRandRange(-TurnRate, TurnRate) * DeltaTime);
	}

	Pulse.Location += Pulse.Direction * MoveSpeed * DeltaTime;

	FVector2D const Center(GetOwner()->GetActorLocation());
	if (FVector2D::DistSquared(Pulse.Location, Center) > AreaRadius * AreaRadius)
	{
		Pulse.Direction = (Center - Pulse.Location).GetSafeNormal();
	}
}

void UGeoBackgroundPulseComponent::SetSlot(int32 SlotIndex, FLinearColor Value)
{
	UKismetMaterialLibrary::SetVectorParameterValue(this, PulseCollection, GetSlotParameterName(SlotIndex), Value);
}
