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

	/**
	 * Re-binds the widget to the owner's ASC once it is ready. The owner's ASC may be null when the widget is first
	 * created (player ASCs arrive later via InitGAS/OnRep_PlayerState), so the owner calls this once the ASC exists.
	 * No-op if the widget does not exist yet (InitWidget binds it on creation) or the ASC is still null.
	 */
	void InitializeForOwner();

protected:
	/** Engine widget-creation hook; binds the freshly created widget to the owner's ASC. */
	virtual void InitWidget() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	/** Binds the (already-created) widget to the owner's ASC if both exist. No-op otherwise. */
	void BindWidgetToOwnerASC();

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
};
