// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/Turret/GeoTurret.h"
#include "CoreMinimal.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"

#include "GeoTestTurret.generated.h"

/** Turret carrying working DefaultAttributes (like AGeoTestDeployable) and a public FindBestTarget. */
UCLASS(NotPlaceable, Transient)
class AGeoTestTurret : public AGeoTurret
{
	GENERATED_BODY()

public:
	AGeoTestTurret(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
	{
		Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponent())->DefaultAttributes =
			UGeoTestDeployableAttributes::StaticClass();
	}

	AActor* GetBestTarget() const { return FindBestTarget(); }
};
