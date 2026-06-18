// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/WidgetComponent.h"
#include "CoreMinimal.h"
#include "HUD/Interface/GeoCombattantWidgetHost.h"

#include "GeoCombattantWidgetComp.generated.h"

/**
 * WidgetComponent that automatically initializes its UGenericCombattantWidget with the owning actor's ASC on InitWidget.
 * Re-binding is safe and idempotent — call BindToOwnerASC again once the ASC is available.
 * Add to any AGeoCharacter or AGeoInteractableActor that should display a world-space health bar.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITYUI_API UGeoCombattantWidgetComp
	: public UWidgetComponent
	, public IGeoCombattantWidgetHost
{
	GENERATED_BODY()

public:
	/** Validates at construction that the widget class derives from UGenericCombattantWidget. */
	virtual void PostInitProperties() override;
	/** Delegates to Super then calls BindToOwnerASC; may be a no-op if the ASC is not yet ready. */
	virtual void InitWidget() override;

	/** Binds the (already-created) widget to the owner's ASC if both exist. No-op otherwise. */
	virtual void BindToOwnerASC() const override;

protected:
	/** Calls UnbindStatCallbacks on the widget before delegating to Super. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;


public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
};
