// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoHealthThresholdTrigger.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/GeoCharacter.h"
#include "EngineUtils.h"
#include "GeoTrinity/GeoTrinity.h"

void UGeoHealthThresholdTrigger::OnArm_Implementation()
{
	WasPast.Reset();
	Crossings = 0;
	ensureMsgf(Watched.Class, TEXT("%s: nothing watched"), *GetName());
	ActorSpawnedHandle = GetWorld()->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UGeoHealthThresholdTrigger::OnActorSpawned));
	for (TActorIterator<AGeoCharacter> It(GetWorld()); It && ActorSpawnedHandle.IsValid(); ++It)
	{
		if (Watched.Matches(**It))
		{
			Watch(**It);
		}
	}
}

void UGeoHealthThresholdTrigger::OnDisarm_Implementation()
{
	GetWorld()->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	ActorSpawnedHandle.Reset();
	for (TPair<TWeakObjectPtr<AActor>, bool> const& Entry : WasPast)
	{
		UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Entry.Key.Get());
		if (ASC)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetHealthAttribute()).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetMaxHealthAttribute()).RemoveAll(this);
		}
	}

	WasPast.Reset();
}

void UGeoHealthThresholdTrigger::Watch(AActor& Actor)
{
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(&Actor);
	if (ASC)
	{
		WasPast.Add(&Actor, false);
		ASC->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetHealthAttribute())
			.AddUObject(this, &UGeoHealthThresholdTrigger::OnHealthOrMaxHealthChanged, TWeakObjectPtr<AActor>(&Actor));
		ASC->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetMaxHealthAttribute())
			.AddUObject(this, &UGeoHealthThresholdTrigger::OnHealthOrMaxHealthChanged, TWeakObjectPtr<AActor>(&Actor));
		Evaluate(Actor);
	}
	else
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: %s has no ability system to watch"), *GetName(), *Actor.GetName());
	}
}

void UGeoHealthThresholdTrigger::OnActorSpawned(AActor* const Actor)
{
	if (Actor)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
			this, &UGeoHealthThresholdTrigger::WatchIfMatching, TWeakObjectPtr<AActor>(Actor)));
	}
}

void UGeoHealthThresholdTrigger::WatchIfMatching(TWeakObjectPtr<AActor> const Actor)
{
	if (ActorSpawnedHandle.IsValid() && Actor.IsValid() && Watched.Matches(*Actor))
	{
		Watch(*Actor);
	}
}

void UGeoHealthThresholdTrigger::OnHealthOrMaxHealthChanged(FOnAttributeChangeData const& /*Data*/,
															TWeakObjectPtr<AActor> const Actor)
{
	if (Actor.IsValid())
	{
		Evaluate(*Actor);
	}
}

void UGeoHealthThresholdTrigger::Evaluate(AActor& Actor)
{
	UGeoAbilitySystemComponent const* const ASC = GeoASLib::GetGeoAscFromActor(&Actor);
	float const Health = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
	float const MaxHealth = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	float const Current = MaxHealth > 0.f ? Health / MaxHealth : 0.f;

	AGeoCharacter const* const Character = Cast<AGeoCharacter>(&Actor);
	bool const bAlive = !Character || !Character->IsDead();

	bool const bPast =
		bAlive && MaxHealth > 0.f && ((Side == EGeoHealthSide::Above) ? Current >= Ratio : Current <= Ratio);
	bool& bWasPast = WasPast.FindOrAdd(&Actor);
	bool const bCrossed = bPast && !bWasPast;
	bWasPast = bPast;
	if (bCrossed)
	{
		FGeoEncounterPayload Payload;
		Payload.Actor = &Actor;
		Payload.Count = ++Crossings;
		Payload.Value = Current;
		Fire(Payload);
	}
}
