// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/ZoneLayer/GeoZoneLayerSubsystem.h"

#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneLayerActor.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

bool UGeoZoneLayerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && !IsRunningDedicatedServer();
}

void UGeoZoneLayerSubsystem::Tick(float const DeltaTime)
{
	Super::Tick(DeltaTime);

	// A play-in-editor dedicated server shares the process with its clients: only its world skips drawing.
	if (GeoLib::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	if (!IsValid(LayerActor))
	{
		SpawnLayerActor();
	}

	PackSlots();
	UploadSlotTexture();
	if (IsValid(LayerActor))
	{
		LayerActor->SetZoneCount(ZoneCount);
	}
}

void UGeoZoneLayerSubsystem::Register(UGeoZoneVisualComponent* const Component)
{
	Shapes.AddUnique(Component);
}

void UGeoZoneLayerSubsystem::Unregister(UGeoZoneVisualComponent* const Component)
{
	Shapes.Remove(Component);
}

void UGeoZoneLayerSubsystem::PackSlots()
{
	SlotTexels.Init(FLinearColor::Transparent, GeoZoneLayer::MaxZoneSlots * GeoZoneLayer::SlotRows);
	Shapes.RemoveAll(
		[](TWeakObjectPtr<UGeoZoneVisualComponent> const& Shape)
		{
			return !Shape.IsValid();
		});
	ZoneCount = 0;
	for (TWeakObjectPtr<UGeoZoneVisualComponent> const& Shape : Shapes)
	{
		AActor const* const Owner = Shape->GetOwner();
		if (!Owner || !Owner->IsHidden())
		{
			if (!ensureMsgf(ZoneCount < GeoZoneLayer::MaxZoneSlots,
							TEXT("%hs: over %d zone shapes at once, the rest are not drawn"), __FUNCTION__,
							GeoZoneLayer::MaxZoneSlots))
			{
				break;
			}

			GeoZoneLayer::FSlotTexels const Texels = GeoZoneLayer::PackSlot(*Shape);
			for (int32 Row = 0; Row < GeoZoneLayer::SlotRows; ++Row)
			{
				SlotTexels[Row * GeoZoneLayer::MaxZoneSlots + ZoneCount] = Texels[Row];
			}

			++ZoneCount;
		}
	}
}

GeoZoneLayer::FSlotTexels UGeoZoneLayerSubsystem::GetSlotTexels(int32 const Slot) const
{
	GeoZoneLayer::FSlotTexels Texels;
	for (int32 Row = 0; Row < GeoZoneLayer::SlotRows; ++Row)
	{
		Texels[Row] = SlotTexels[Row * GeoZoneLayer::MaxZoneSlots + Slot];
	}

	return Texels;
}

void UGeoZoneLayerSubsystem::SpawnLayerActor()
{
	if (!SlotTexture)
	{
		SlotTexture = UTexture2D::CreateTransient(GeoZoneLayer::MaxZoneSlots, GeoZoneLayer::SlotRows, PF_A32B32G32R32F,
												  TEXT("GeoZoneSlots"));
		// Point sampled and linear: every texel is raw data.
		SlotTexture->Filter = TF_Nearest;
		SlotTexture->AddressX = TA_Clamp;
		SlotTexture->AddressY = TA_Clamp;
		SlotTexture->SRGB = false;
		SlotTexture->UpdateResource();
	}

	FActorSpawnParameters SpawnParameters;
	// Never saved with an editor world's level, never listed in its outliner.
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.bHideFromSceneOutliner = true;
	LayerActor = GetWorld()->SpawnActor<AGeoZoneLayerActor>(SpawnParameters);
	if (ensureMsgf(LayerActor, TEXT("%hs: the zone layer actor would not spawn"), __FUNCTION__))
	{
		LayerActor->InitPlanes(SlotTexture);
	}
}

void UGeoZoneLayerSubsystem::UploadSlotTexture() const
{
	int32 const DataSize = SlotTexels.Num() * sizeof(FLinearColor);
	// Read on the render thread after this returns: the copy and the region are freed there.
	uint8* const Data = new uint8[DataSize];
	FMemory::Memcpy(Data, SlotTexels.GetData(), DataSize);
	FUpdateTextureRegion2D* const Region =
		new FUpdateTextureRegion2D(0, 0, 0, 0, GeoZoneLayer::MaxZoneSlots, GeoZoneLayer::SlotRows);
	SlotTexture->UpdateTextureRegions(0, 1, Region, GeoZoneLayer::MaxZoneSlots * sizeof(FLinearColor),
									  sizeof(FLinearColor), Data,
									  [](uint8* SourceData, FUpdateTextureRegion2D const* Regions)
									  {
										  delete[] SourceData;
										  delete Regions;
									  });
}
