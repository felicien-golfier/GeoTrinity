// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/FatalZonePattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void UFatalZonePattern::OnCreate(FGameplayTag AbilityTag)
{
	Super::OnCreate(AbilityTag);
	// TODO: why not having deployables in the pooling system. Need to set it up properly
	//  UGeoActorPoolingSubsystem::Get(GetWorld())->PreSpawn(PillarClass, 10);
}

void UFatalZonePattern::InitPattern(FAbilityPayload const& Payload)
{
	Super::InitPattern(Payload);

	float const TimeUntilExpire = FMath::Max(0.f, CountdownDuration - CalculateElapsedTime());

	if (CountdownGameplayCueTag.IsValid())
	{
		UGeoAbilitySystemComponent* ASC =
			Payload.Owner ? Payload.Owner->FindComponentByClass<UGeoAbilitySystemComponent>() : nullptr;

		if (ASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = FVector(Payload.Origin, 0.f);
			CueParams.Instigator = Payload.Instigator;
			CueParams.AbilityLevel = Payload.AbilityLevel;
			CueParams.RawMagnitude = CountdownDuration;
			CueParams.NormalizedMagnitude = TimeUntilExpire / CountdownDuration;
			CueParams.Normal = FVector(ZoneSize, 0.f, 0.f);
			ASC->ExecuteGameplayCue(CountdownGameplayCueTag, CueParams);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(ExpiryTimerHandle, this, &UFatalZonePattern::OnExpire, TimeUntilExpire,
										   false);
}

void UFatalZonePattern::OnExpire()
{
	FVector const ZoneLocation = FVector(StoredPayload.Origin, ArbitraryCharacterZ);
	UGeoAbilitySystemComponent* OwnerASC =
		StoredPayload.Owner ? StoredPayload.Owner->FindComponentByClass<UGeoAbilitySystemComponent>() : nullptr;

	if (ExpiryGameplayCueTag.IsValid())
	{
		if (!ensureMsgf(OwnerASC, TEXT("UFatalZonePattern::TickPattern — OwnerASC is null, cannot execute expiry cue")))
		{
			EndPattern();
			return;
		}

		FGameplayCueParameters CueParams;
		CueParams.Location = ZoneLocation;
		CueParams.Instigator = StoredPayload.Instigator;
		CueParams.AbilityLevel = StoredPayload.AbilityLevel;
		CueParams.RawMagnitude = ZoneSize;
		OwnerASC->ExecuteGameplayCue(ExpiryGameplayCueTag, CueParams);
	}

	UWorld* World = GetWorld();
	if (UGeoGameplayLibrary::IsServer(World))
	{
		if (OwnerASC && ZoneEffectDataArray.Num() > 0)
		{
			FVector2D const Origin2D(ZoneLocation.X, ZoneLocation.Y);
			for (AActor* TargetActor : GeoASLib::GetInteractableActors(
					 this, GeoASLib::GetTeamId(StoredPayload.Owner), static_cast<int32>(ETeamAttitudeBitflag::Hostile),
					 true, Origin2D, ZoneSize))
			{
				UGeoAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UGeoAbilitySystemComponent>();
				if (TargetASC)
				{
					UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(ZoneEffectDataArray, OwnerASC, TargetASC,
																		StoredPayload.AbilityLevel, StoredPayload.Seed);
				}
			}
		}

		if (!IsValid(PillarClass))
		{
			ensureMsgf(false, TEXT("PillarClass null, please fill the class in the FatalZonePattern"));
			return;
		}

		AGeoPillar* Pillar = Cast<AGeoPillar>(UGeoActorPoolingSubsystem::Get(World)->RequestActor(
			PillarClass, FTransform(ZoneLocation), StoredPayload.Owner, Cast<APawn>(StoredPayload.Instigator), false,
			false));

		if (IsValid(Pillar))
		{
			FDeployableData PillarData;
			PillarData.Owner = StoredPayload.Owner;
			PillarData.Instigator = StoredPayload.Instigator;
			PillarData.Level = StoredPayload.AbilityLevel;
			PillarData.Seed = StoredPayload.Seed;
			PillarData.EffectDataArray = PillarEffectDataArray;
			PillarData.Params.Size = ZoneSize;

			Pillar->InitInteractable(&PillarData);
			UGeoActorPoolingSubsystem::Get(World)->ChangeActorState(Pillar, true);
			if (Pillar->GetClass()->ImplementsInterface(UGeoPoolableInterface::StaticClass()))
			{
				CastChecked<IGeoPoolableInterface>(Pillar)->Init();
			}
		}
	}

	EndPattern();
}
