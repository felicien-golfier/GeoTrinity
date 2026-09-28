// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/ZonePattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Characters/Component/GeoIndicatorComponent.h"
#include "NiagaraComponent.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void UZonePattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);
	bHasHazard = ZoneParams.LifeDrainMaxDuration <= 0.f;
}

void UZonePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);

	bool const bIsWindingUp = GetWorld()->GetTimerManager().IsTimerActive(StartSectionTimerHandle);
	if (bIsWindingUp)
	{
		FGeoIndicatorState State;
		State.Location = FVector(StoredPayload.Origin, ArbitraryCharacterZ);
		State.Size.X = ZoneParams.Size;
		State.Colors = GeoColor::GetMeaningColors(ZoneParams.Color, ZoneParams.SecondaryColors);
		State.StartServerTime = StoredPayload.ServerSpawnTime;
		State.Duration = StartDelay;
		IndicatorComponent = UGeoIndicatorComponent::SpawnIndicator(this, State);
	}
}

void UZonePattern::StartPattern()
{
	RemoveIndicator();
	Super::StartPattern();

	if (!bHasHazard)
	{
		if (GeoLib::IsServer(GetWorld()))
		{
			GeoASLib::FullySpawnDeployable(GetZoneClass(), StoredPayload, EffectDataArray, ZoneParams,
										   FTransform(FVector(StoredPayload.Origin, ArbitraryCharacterZ)));
		}

		EndPattern();
	}
}

bool UZonePattern::IsInHazard(AActor const* Target, FVector2D const Location, float /*SpentTime*/) const
{
	return GeoASLib::IsInCircle(Target, Location, StoredPayload.Origin, ZoneParams.Size, ETargetOverlapMode::Automatic,
								GeoASLib::GetTeamId(StoredPayload.SourceOwner));
}

void UZonePattern::EndPattern(bool const bForceStop)
{
	RemoveIndicator();
	Super::EndPattern(bForceStop);
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

void UZonePattern::RemoveIndicator()
{
	if (IsValid(IndicatorComponent))
	{
		IndicatorComponent->DestroyComponent();
	}

	IndicatorComponent = nullptr;
}
