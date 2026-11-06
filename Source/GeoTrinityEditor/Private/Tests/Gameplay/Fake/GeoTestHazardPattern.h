// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/DevastatingWavePattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Tool/Visual/GeoNiagaraParams.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

#include "GeoTestHazardPattern.generated.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GeoTestMask
{
	/** A transient stand-in for MPC_MaskedArea: every parameter FSafeSpotMask::Set writes, slots on the "free" default.
	 */
	inline UMaterialParameterCollection* MakeCollection()
	{
		UMaterialParameterCollection* const Collection = NewObject<UMaterialParameterCollection>(GetTransientPackage());
		auto const AddVector = [Collection](FName const Name, FLinearColor const& Default)
		{
			FCollectionVectorParameter Parameter;
			Parameter.ParameterName = Name;
			Parameter.Id = FGuid::NewGuid();
			Parameter.DefaultValue = Default;
			Collection->VectorParameters.Add(Parameter);
		};
		for (int32 SlotIndex = 0; SlotIndex < FSafeSpotMask::MaxSlots; ++SlotIndex)
		{
			AddVector(GeoMaterialParams::GetPillarPositionName(SlotIndex),
					  FLinearColor(-10000.f, -10000.f, -10000.f, 0.f));
		}
		AddVector(GeoMaterialParams::SafeSpotLaneDirection, FLinearColor::Black);
		AddVector(GeoMaterialParams::SafeSpotOrigin, FLinearColor::Black);

		for (FName const Name : {GeoMaterialParams::PillarRadius, GeoMaterialParams::SafeSpotShadowMode})
		{
			FCollectionScalarParameter Parameter;
			Parameter.ParameterName = Name;
			Parameter.Id = FGuid::NewGuid();
			Collection->ScalarParameters.Add(Parameter);
		}

		return Collection;
	}

	/** The slot value the world's instance of Collection holds now (alpha 1 = a spot is in it, 0 = free). */
	inline FLinearColor ReadSlot(UWorld& World, UMaterialParameterCollection& Collection, int32 const SlotIndex)
	{
		FLinearColor Value(ForceInitToZero);
		World.GetParameterCollectionInstance(&Collection)
			->GetVectorParameterValue(GeoMaterialParams::GetPillarPositionName(SlotIndex), Value);
		return Value;
	}

	/** How many slots of the world's instance of Collection hold a spot. */
	inline int32 CountUsedSlots(UWorld& World, UMaterialParameterCollection& Collection)
	{
		int32 Used = 0;
		for (int32 SlotIndex = 0; SlotIndex < FSafeSpotMask::MaxSlots; ++SlotIndex)
		{
			Used += ReadSlot(World, Collection, SlotIndex).A > 0.5f ? 1 : 0;
		}

		return Used;
	}
} // namespace GeoTestMask

#endif

/** Lateral wave of a test: a 10 s wind-up the test never waits for, a mask of its own. Brings the zone layer shape
 * UZoneHazardPattern::OnCreate would, since that one reads an ability CDO a bare test pattern has none of. */
UCLASS(NotPlaceable, Transient)
class UGeoTestLateralWavePattern : public ULateralWavePattern
{
	GENERATED_BODY()

public:
	virtual void OnCreate(FGameplayTag, AActor& Owner) override
	{
		StartDelay = 10.f;
		bHasHazard = true;
		MaskMaterialParameterCollection = GeoTestMask::MakeCollection();
		if (!GetWorld()->IsNetMode(NM_DedicatedServer))
		{
			HazardVisual = NewObject<UGeoZoneVisualComponent>(&Owner);
			HazardVisual->SetupAttachment(Owner.GetRootComponent());
			HazardVisual->SetAbsolute(true, true, true);
			HazardVisual->SetZoneVisible(false);
			HazardVisual->RegisterComponent();
		}
	}

	UMaterialParameterCollection* GetMask() const { return MaskMaterialParameterCollection; }

	/** The hit test at Location, SpentTime seconds after the front left. */
	bool IsHitAt(FVector2D const Location, float const SpentTime) const
	{
		return IsInHazard(nullptr, Location, SpentTime);
	}

	/** How long the hazard lasts, from the front leaving to the band leaving the travel. */
	float GetDuration() const { return GetHazardDuration(); }

	/** Stops the firing, as the ability ending does. */
	void StopFiring() { EndPattern(true); }
};

/** Devastating wave of a test: same set-up as UGeoTestLateralWavePattern. */
UCLASS(NotPlaceable, Transient)
class UGeoTestDevastatingWavePattern : public UDevastatingWavePattern
{
	GENERATED_BODY()

public:
	virtual void OnCreate(FGameplayTag, AActor& Owner) override
	{
		StartDelay = 10.f;
		bHasHazard = true;
		MaskMaterialParameterCollection = GeoTestMask::MakeCollection();
		if (!GetWorld()->IsNetMode(NM_DedicatedServer))
		{
			HazardVisual = NewObject<UGeoZoneVisualComponent>(&Owner);
			HazardVisual->SetupAttachment(Owner.GetRootComponent());
			HazardVisual->SetAbsolute(true, true, true);
			HazardVisual->SetZoneVisible(false);
			HazardVisual->RegisterComponent();
		}
	}

	UMaterialParameterCollection* GetMask() const { return MaskMaterialParameterCollection; }
};
