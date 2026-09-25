// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Zones/GeoEffectZone.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Tool/GeoNiagaraParams.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoEffectZone::AGeoEffectZone(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowDamageNumbers = false;
	SetCanBeDamaged(false);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoEffectZone, Data, COND_InitialOnly);
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
		ZoneJudge.Start(GeoLib::GetServerTime(GetWorld()), FGeoHazardJudge::UntilEnded,
						/*bSeenThroughReplication*/ true, /*bEndsOnHit*/ false,
						/*bRemovesInfiniteEffectsOnLeave*/ true);
		JudgeZone();
	}
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
			return GeoASLib::IsInCircle(Target, Location, FVector2D(GetActorLocation()), Data.Params.Size,
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
	CapsuleComponent->SetCapsuleHalfHeight(Data.Params.Size);
	CapsuleComponent->SetCapsuleRadius(Data.Params.Size);

	for (UMeshComponent* const MeshComponent : GetVisualMeshComponents())
	{
		FVector Scale = MeshComponent->GetRelativeScale3D();
		Scale.X = Data.Params.Size * .02f;
		Scale.Y = Data.Params.Size * .02f;
		MeshComponent->SetRelativeScale3D(Scale);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::ApplyColor() const
{
	TArray<FLinearColor> const Colors = GeoColor::GetMeaningColors(Data.Params.Color, Data.Params.SecondaryColors);
	for (UMeshComponent* const MeshComponent : GetVisualMeshComponents())
	{
		for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			// Returns the existing instance when the slot already holds one, so repeated calls make no new material.
			// Null only for an empty material slot, which the engine already warns about.
			UMaterialInstanceDynamic* const Material =
				MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
			if (!Material)
			{
				continue;
			}
			Material->SetVectorParameterValue(GeoMaterialParams::ZoneOutlineColor, Colors[0]);
			for (int32 Index = 0; Index < Colors.Num(); ++Index)
			{
				Material->SetVectorParameterValue(GeoMaterialParams::ZoneInsideColors[Index], Colors[Index]);
			}

			Material->SetScalarParameterValue(GeoMaterialParams::ZoneColorCount, Colors.Num());
		}
	}
}
