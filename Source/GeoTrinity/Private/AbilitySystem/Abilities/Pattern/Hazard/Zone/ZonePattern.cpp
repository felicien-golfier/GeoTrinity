// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/ZonePattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Characters/Component/FX/GeoIndicatorComponent.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

void UZonePattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);
	bHasHazard = LandsAsBurst();
}

void UZonePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);

	bool const bIsWindingUp = GetWorld()->GetTimerManager().IsTimerActive(StartSectionTimerHandle);
	if (bIsWindingUp && ZoneParams.Size > 0)
	{
		FGeoIndicatorState State;
		State.Size.X = ZoneParams.Size;
		State.ColorMask = GeoZoneLayer::MakeColorMask(ZoneParams.Color, ZoneParams.SecondaryColors);
		State.StartServerTime = StoredPayload.ServerSpawnTime;
		State.Duration = StartDelay;
		for (FVector2D const& Location : GetTargetLocations())
		{
			State.Location = FVector(Location, ArbitraryCharacterZ);
			IndicatorComponents.Add(UGeoIndicatorComponent::SpawnIndicator(this, State));
		}
	}
}

void UZonePattern::StartPattern()
{
	RemoveIndicators();
	SpawnedZones.Reset();
	Super::StartPattern();

	if (!bHasHazard)
	{
		if (GeoLib::IsServer(GetWorld()))
		{
			for (FVector2D const& Location : GetTargetLocations())
			{
				SpawnedZones.Add(GeoASLib::FullySpawnDeployable(GetZoneClass(), StoredPayload, EffectDataArray,
																ZoneParams,
																FTransform(FVector(Location, ArbitraryCharacterZ))));
			}
		}

		if (!bZoneUntilStopped)
		{
			EndPattern();
		}
	}
}

bool UZonePattern::LandsAsBurst() const
{
	return ZoneParams.LifeDrainMaxDuration <= 0.f && !bZoneUntilStopped;
}

bool UZonePattern::IsInHazard(AActor const* Target, FVector2D const Location, float /*SpentTime*/) const
{
	FGenericTeamId const SourceTeam = GeoASLib::GetTeamId(StoredPayload.SourceOwner);
	for (FVector2D const& ZoneLocation : GetTargetLocations())
	{
		if (GeoASLib::IsInCircle(Target, Location, ZoneLocation, ZoneParams.Size, ETargetOverlapMode::Automatic,
								 SourceTeam))
		{
			return true;
		}
	}

	return false;
}

void UZonePattern::EndPattern(bool const bForceStop)
{
	RemoveIndicators();
	if (bForceStop && bZoneUntilStopped)
	{
		for (TWeakObjectPtr<AGeoDeployableBase> const& Zone : SpawnedZones)
		{
			if (Zone.IsValid())
			{
				Zone->Recall();
			}
		}

		SpawnedZones.Reset();
	}

	Super::EndPattern(bForceStop);
}

void UZonePattern::ExecuteGameplayCue(FGeoCueParam const& Cue)
{
	if (!Cue.IsValid() || GeoLib::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	UGeoAbilitySystemComponent* const AvatarASC = GeoASLib::GetGeoAscFromActor(StoredPayload.SourceAvatar);
	if (!ensureMsgf(IsValid(AvatarASC), TEXT("Pattern %s: source avatar has no ASC"), *GetName()))
	{
		return;
	}

	for (FVector2D const& Location : GetTargetLocations())
	{
		FGameplayCueParameters CueParams = FillCueParam(Cue, StoredPayload);
		CueParams.Location = FVector(Location, 0.f);
		GeoASLib::ExecuteGeoCue(AvatarASC, Cue, CueParams, true);
	}
}

FGameplayCueParameters UZonePattern::FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Cue, Payload);
	CueParams.RawMagnitude = ZoneParams.Size;
	return CueParams;
}

TSubclassOf<AGeoDeployableBase> UZonePattern::GetZoneClass() const
{
	if (ZoneClass)
	{
		return ZoneClass;
	}

	TSubclassOf<AGeoDeployableBase> const DefaultZoneClass =
		GetDefault<UGameDataSettings>()->DefaultZoneClass.LoadSynchronous();
	ensureMsgf(DefaultZoneClass, TEXT("%hs: no DefaultZoneClass in Game Data Settings and %s names no ZoneClass"),
			   __FUNCTION__, *GetName());
	return DefaultZoneClass;
}

void UZonePattern::RemoveIndicators()
{
	for (UGeoZoneVisualComponent* const Indicator : IndicatorComponents)
	{
		if (IsValid(Indicator))
		{
			Indicator->DestroyComponent();
		}
	}

	IndicatorComponents.Reset();
}
