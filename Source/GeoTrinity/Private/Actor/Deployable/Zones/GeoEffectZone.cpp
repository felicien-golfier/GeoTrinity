// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Zones/GeoEffectZone.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

AGeoEffectZone::AGeoEffectZone(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowDamageNumbers = false;
	SetCanBeDamaged(false);
	Color.Color = EGeoColor::Neutral;
	ZoneVisual = CreateDefaultSubobject<UGeoZoneVisualComponent>(TEXT("ZoneVisual"));
	ZoneVisual->SetupAttachment(GetRootComponent());
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoEffectZone, Data, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGeoEffectZone, GrowthStartServerTime, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::InitInteractable(FInteractableActorData* InputData)
{
	FDeployableData* const DeployableData = static_cast<FDeployableData*>(InputData);
	if (!ensureMsgf(DeployableData, TEXT("AGeoEffectZone: Data is not an FDeployableData!")))
	{
		return;
	}
	Data = *DeployableData;
	ApplyRadius();

	Super::InitInteractable(InputData);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);

	SetActorRotation(FRotator::ZeroRotator);

	// A spawned zone is already initialized by the time OnConstruction runs (FinishSpawning comes after
	// InitInteractable), so only a placed one still needs its Details-panel fields pushed into Data.
	if (!Data.Owner)
	{
		Data.Params.Size = Radius;
		Data.Params.Color = Color;
		Data.Params.SecondaryColors = SecondaryColors;
		Data.Params.Attitude = AttitudeBitmask;
		Data.EffectDataArray = EffectDataArray;
	}
	ApplyRadius();
	ApplyColor();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::BeginPlay()
{
	// Hand-placed: no spawner calls InitInteractable, so initialize GAS here before Super inits default attributes.
	if (!Data.Owner)
	{
		Data.Owner = this;
		Data.Instigator = this;
		Data.TeamID = FGenericTeamId(static_cast<uint8>(Team));
		Data.Level = Level;
		Data.Params.Size = Radius;
		Data.Params.Color = Color;
		Data.Params.SecondaryColors = SecondaryColors;
		Data.Params.Attitude = AttitudeBitmask;
		Data.EffectDataArray = EffectDataArray;
		InitGas(Data.Owner);
	}

	Super::BeginPlay();
	SetActorRotation(FRotator::ZeroRotator);
	ApplyRadius();
	ApplyColor();

	if (GeoLib::IsServer(GetWorld()))
	{
		GrowthStartServerTime = GeoLib::GetServerTime(GetWorld());
		ZoneJudge.Start(GeoLib::GetServerTime(GetWorld()), FGeoHazardJudge::UntilEnded,
						/*bSeenThroughReplication*/ true, /*bEndsOnHit*/ false,
						/*bRemovesInfiniteEffectsOnLeave*/ true);
		JudgeZone();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Data.Params.GrowToSize.GetValueAtLevel(Data.Level) > 0.f)
	{
		float const Seconds = Data.Params.GrowSeconds.GetValueAtLevel(Data.Level);
		float const Alpha = Seconds > 0.f
			? FMath::Clamp((GeoLib::GetServerTime(GetWorld()) - GrowthStartServerTime) / Seconds, 0.f, 1.f)
			: 1.f;
		if (Alpha != GrowthAlpha)
		{
			GrowthAlpha = Alpha;
			ApplyRadius();
		}
	}

	ZoneVisual->Fill = 1.f - GetDrainDurationRatio();
}

// ---------------------------------------------------------------------------------------------------------------------
float AGeoEffectZone::GetRadius() const
{
	return FMath::Lerp(Data.Params.Size, Data.Params.GrowToSize.GetValueAtLevel(Data.Level), GrowthAlpha);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	ZoneJudge.Stop();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::JudgeZone()
{
	float const ServerTime = GeoLib::GetServerTime(GetWorld());
	if (!IsActive() || IsBlinking())
	{
		ZoneJudge.EndAt(ServerTime);
	}

	OnZoneJudged(ZoneJudge.Judge(
		MakeHazardSource(), Data.EffectDataArray, FGeoHazardJudge::FindCandidates(Data.Owner, Data.Params.Attitude),
		[this](AActor const* Target, FVector2D const Location, float /*SpentTime*/)
		{
			return GeoASLib::IsInCircle(Target, Location, FVector2D(GetActorLocation()), GetRadius(),
										ETargetOverlapMode::IncludeRadius, GeoASLib::GetTeamId(Data.Owner));
		}));

	if (!ZoneJudge.IsOver(ServerTime))
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::JudgeZone);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnRep_Data()
{
	ApplyRadius();
	ApplyColor();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::ApplyRadius() const
{
	float const ZoneRadius = GetRadius();
	CapsuleComponent->SetCapsuleHalfHeight(ZoneRadius);
	CapsuleComponent->SetCapsuleRadius(ZoneRadius);

	ZoneVisual->Shape.Radius = ZoneRadius;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::ApplyColor() const
{
	ZoneVisual->ColorMask = GeoZoneLayer::MakeColorMask(Data.Params.Color, Data.Params.SecondaryColors);
}
