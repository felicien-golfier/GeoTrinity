// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/WidgetComponent.h"
#include "CoreMinimal.h"

#include "GeoCombattantWidgetComp.generated.h"

/**
 * WidgetComponent that automatically initializes its UGenericCombattantWidget with the owning actor's ASC on BeginPlay.
 * Add to any AGeoCharacter or AGeoInteractableActor that should display a world-space health bar.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoCombattantWidgetComp : public UWidgetComponent
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
};
