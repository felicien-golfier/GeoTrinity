// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/ZoneLayer/GeoZoneLayerActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Settings/GameDataSettings.h"
#include "Tool/Visual/GeoColor.h"
#include "Tool/Visual/GeoNiagaraParams.h"
#include "UObject/ConstructorHelpers.h"
#include "World/ZoneLayer/GeoZoneShape.h"

/** World cm each plane spans, centred on the origin: past any arena. */
static constexpr float LayerSize = 1000000.f;

AGeoZoneLayerActor::AGeoZoneLayerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> const PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	GroundPlane = CreatePlane(TEXT("GroundPlane"), GeoZoneLayer::GroundSortPriority, PlaneMesh.Object);
	TelegraphPlane = CreatePlane(TEXT("TelegraphPlane"), GeoZoneLayer::TelegraphSortPriority, PlaneMesh.Object);
}

UStaticMeshComponent* AGeoZoneLayerActor::CreatePlane(FName const Name, int32 const SortPriority,
													  UStaticMesh* const Mesh)
{
	UStaticMeshComponent* const Plane = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Plane->SetupAttachment(RootComponent);
	Plane->SetStaticMesh(Mesh);
	Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plane->SetCastShadow(false);
	Plane->SetTranslucentSortPriority(SortPriority);
	Plane->SetVisibility(false);
	// Spans the editor viewport too: a click there selects what lies under the layer.
	Plane->bSelectable = false;
	Plane->SetRelativeLocation(FVector(0.f, 0.f, GeoZoneLayer::ZoneLayerZ));
	// The engine plane is 100 cm wide.
	Plane->SetRelativeScale3D(FVector(LayerSize / 100.f, LayerSize / 100.f, 1.f));
	return Plane;
}

void AGeoZoneLayerActor::InitPlanes(UTexture2D* const SlotTexture)
{
	UGameDataSettings const* const Settings = GetDefault<UGameDataSettings>();
	UTexture2D* const Palette = GeoColor::CreatePaletteTexture();
	TPair<UStaticMeshComponent*, TSoftObjectPtr<UMaterialInterface>> const Planes[] = {
		{GroundPlane, Settings->ZoneLayerGroundMaterial},
		{TelegraphPlane, Settings->ZoneLayerTelegraphMaterial}};
	for (auto const& [Plane, Material] : Planes)
	{
		UMaterialInterface* const LoadedMaterial = UGameDataSettings::GetLoadedDataAsset(Material);
		if (ensureMsgf(LoadedMaterial, TEXT("%hs: no %s material in Game Data Settings, its zones are not drawn"),
					   __FUNCTION__, *Plane->GetName()))
		{
			UMaterialInstanceDynamic* const Instance = Plane->CreateDynamicMaterialInstance(0, LoadedMaterial);
			Instance->SetTextureParameterValue(GeoMaterialParams::ZoneSlots, SlotTexture);
			Instance->SetTextureParameterValue(GeoMaterialParams::ZonePalette, Palette);
			Instance->SetScalarParameterValue(GeoMaterialParams::ZoneCount, 0.f);
			Plane->SetVisibility(true);
		}
	}
}

void AGeoZoneLayerActor::SetZoneCount(int32 const ZoneCount) const
{
	for (UStaticMeshComponent const* const Plane : {GroundPlane.Get(), TelegraphPlane.Get()})
	{
		if (UMaterialInstanceDynamic* const Instance = Cast<UMaterialInstanceDynamic>(Plane->GetMaterial(0)))
		{
			Instance->SetScalarParameterValue(GeoMaterialParams::ZoneCount, static_cast<float>(ZoneCount));
		}
	}
}
